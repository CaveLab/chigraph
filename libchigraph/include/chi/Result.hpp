/// \file chi/Result.hpp
/// Defines the Result class and related functions

#ifndef CHI_RESULT_HPP
#define CHI_RESULT_HPP

#pragma once

#include "chi/json.hpp"

#include <boost/container/flat_map.hpp>

namespace chi {
/// The result object, used for identifiying errors with good diagnostics
///
/// ## Usage:
/// If you want to construct a default result object, just call the default constructor:
/// ```
/// Result res;
/// ```
/// `res` is the most common name for an error object, prefer it to other names.
///
/// When an error, warning, or just some event that requries logging needs to be added to the result
/// object,
/// call `Result::addEntry`:
/// ```
/// Result res;
/// res.addEntry("E231", "Some error occured", {{"Line Number", 34}});
/// ```
/// Note the `E` at the beginning of the error code, this is important. If it's an `E`, it's
/// considered an error,
/// and will mark the result as errored. If it's a `W`, then it's logged, but _it's still considred
/// successful_.
///
/// If you're calling another operation that emits a Result object, then there's an easy to
/// integrate that result object: the opeartor+=
///
/// ```
/// Result res;
/// res += connectExec(...); // or any function that returns a Result
/// ```
/// ## Standardized names in data:
/// In order to have good parsing of errors, some standards are good.
/// If you are trying to represent this data in your error, use these
/// standards so it can be parsed later on and presented to the user in a nice way
///
/// Name in JSON  | Description
/// ------------- | -------------
/// `Node ID`     | The ID of the node that errored
///
/// # What is EUKN?
/// Not every error in chigraph is documented and tested. Errors with numbers are tested and
/// possibly documented.
/// EUKN errors are just undocumented errors.
///
/// ## Implementation Details:
/// Result objects store a json object that represents the error metadata.
/// It's an array, each being an object containing three objects:
/// - `errorcode`: The errorcode. This is an identifier representng the error: a
///    character followed by a number. The value of the character changes the behaviour:
///      - `E`: it's an error and sets `success` to false.
///      - `W`: it's a warning
///      - `I`: it's just info
///
///   If it's anything else, that it will cause an assertion
/// - `overview`: A simple string representing the generics of the problem. It *shouldn't* differ
/// from
///   error to error, if you need to include specifics use the `data` element
/// - `data`: Extra metadata for the error, including context and how the error occured.
struct Result {
	/// A helper object for contexts that should be removed at the end of a scope
	struct ScopedContext {
		ScopedContext(Result& res, int ctxId) : result{res}, contextId{ctxId} {}

		~ScopedContext() { result.removeContext(contextId); }

		Result&   result;
		const int contextId;
	};

	/// Default constructor; defaults to success
	Result() : result_json(nlohmann::json::array()) {}
	/// Add a entry to the result, either a warning or an error
	/// \param ec The error/warning code. If it starts with E, then it is an error and success is
	/// set to false, if it starts with a W it's a warning and success can stay true if it is still
	/// true.
	/// \param overview Basic overview of the error, this shouldn't change based on the instance of
	/// the error
	/// \param data The detailed description this instance of the error
	void addEntry(const char* ec, const char* overview, nlohmann::json data);

	/// Add some json that will ALWAYS be merged with entries that are added and when results are
	/// added to this Result
	/// \param data The json to merge with every entry
	/// \return The ID for this context, use this value with removeContext to remove it
	int addContext(const nlohmann::json& data);

	/// Add a context with a scope
	/// Example usage:
	/// ```
	/// chi::Result res;
	/// res.contextJson(); // returns {}
	/// {
	///     auto scopedCtx = res.addScopedContext({{"Module", "lang"}});
	///     res.contextJson(); // returns {"module": "lang"}
	/// }
	/// res.contextJson(); // returns {}
	/// ```
	ScopedContext addScopedContext(const nlohmann::json& data) {
		return ScopedContext{*this, addContext(data)};
	}

	/// Removes a previously added context
	/// \param id The ID for the context added with addContext
	void removeContext(int id);

	/// Get the JSON associated with the context that's been added
	nlohmann::json contextJson() const;

	/// Success test
	operator bool() const { return success(); }

	/// Success test
	bool success() const { return mSuccess; }

	/// !Success test
	bool operator!() const { return !success(); }
	/// Dump to a pretty-printed error message
	std::string dump() const;

	/// if it is successful
	bool mSuccess = true;

	/// The result JSON
	nlohmann::json result_json;

private:
	boost::container::flat_map<int, nlohmann::json> mContexts;
};

/// \name Result operators
/// \{

/// Append two Result objects. Both of the contexts are merged.
/// The entires in lhs have the rhs context applied to them, and viceversa as well
/// \param lhs The left Result
/// \param rhs The right Result
/// \return The concatinated Result
/// \relates Result
Result operator+(const Result& lhs, const Result& rhs);

/// Append one result to an existing one. Both of the contexts are merged.
/// The entires in lhs have the rhs context applied to them, and viceversa as well
/// \param lhs The existing Result to add to
/// \param rhs The (usually temporary) result to be added into lhs.
/// \return *this
/// \relates Result
Result& operator+=(Result& lhs, const Result& rhs);

/// Stream operator
/// \param lhs The stream
/// \param rhs The Result to print to lhs
/// \return lhs after printing
/// \relates Result
inline std::ostream& operator<<(std::ostream& lhs, const Result& rhs) {
	lhs << rhs.dump();

	return lhs;
}

/// \}

}  // namespace chi

#endif  // CHI_RESULT_HPP
