#ifndef SIMDJSON_STREAM_OBJECT_INL_H
#define SIMDJSON_STREAM_OBJECT_INL_H

#include "simdjson/stream/object.h"
#include "simdjson/stream/field.h"

namespace simdjson {
namespace stream {

//
// object
//
really_inline object::object(internal::json_iterator &json) noexcept
  : value(json), depth{json.depth}, at_start{true} {
}

really_inline object::iterator object::begin() noexcept {
  return iterator(*this);
}
really_inline object::iterator object::end() noexcept {
  return iterator(*this);
}
really_inline simdjson_result<element&> object::operator[](std::string_view key) noexcept {
  internal::logger::log_event("lookup key", value.json);
  for (auto [field, error] : *this) {
    if (error || key == field.key()) {
      if (!error) { internal::logger::log_event("found key", value.json); }
      return { value, error };
    }
  }
  internal::logger::log_error("no such key", value.json);
  return { value, NO_SUCH_FIELD };
}

//
// object::iterator
//
really_inline object::iterator::iterator(object &_parent) noexcept
  : parent(_parent) {
}
really_inline simdjson_result<field> object::iterator::operator*() noexcept {
  // Check the comma
  if (parent.at_start) {
    // If we're at the start, there's nothing to check. != would have bailed on empty {}
    internal::logger::log_event("first field", parent.value.json, true);
    parent.at_start = false;
  } else {
    internal::logger::log_event("next field", parent.value.json);
    if (*parent.value.json.advance() != ',') {
      internal::logger::log_error("missing ,", parent.value.json);
      return { field(parent.value.json.get(), parent.value), TAPE_ERROR };
    }
  }

  // Get the key and skip the :
  const uint8_t *key = parent.value.json.advance();
  if (*key != '"') { assert(error); internal::logger::log_error("non-string key", parent.value.json); }
  auto error = (*key == '"' && *parent.value.json.advance() == ':') ? SUCCESS : TAPE_ERROR;
  if (*parent.value.json.peek_prev() != ':') { assert(error); internal::logger::log_error("missing :", parent.value.json); }
  return { field(key, parent.value), error };
}
really_inline object::iterator &object::iterator::operator++() noexcept {
  return *this;
}
really_inline bool object::iterator::operator!=(const object::iterator &) noexcept {
  // Finish the previous value if it wasn't finished already
  if (!parent.at_start) {
    // If finish() fails, it's because it found a stray } or ]
    if (!parent.value.finish(parent.depth)) {
      return true;
    }
  }
  // Stop if we hit }
  if (*parent.value.json.get() == '}') {
    parent.value.json.depth--;
    internal::logger::log_end_event("object", parent.value.json);
    parent.value.json.advance();
    return false;
  }
  return true;
}

} // namespace stream

//
// simdjson_result<stream::object>
//
really_inline simdjson_result<stream::object>::simdjson_result(stream::object &&value) noexcept
    : internal::simdjson_result_base<stream::object>(std::forward<stream::object>(value)) {}
really_inline simdjson_result<stream::object>::simdjson_result(stream::object &&value, error_code error) noexcept
    : internal::simdjson_result_base<stream::object>(std::forward<stream::object>(value), error) {}

really_inline simdjson_result<stream::element&> simdjson_result<stream::object>::operator[](std::string_view key) noexcept {
  if (error()) { return { first.value, error() }; }
  return first[key];
}

#if SIMDJSON_EXCEPTIONS

really_inline stream::object::iterator simdjson_result<stream::object>::begin() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first.begin();
}
really_inline stream::object::iterator simdjson_result<stream::object>::end() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first.end();
}

#endif // SIMDJSON_EXCEPTIONS

} // namespace simdjson

#endif // SIMDJSON_STREAM_DOCUMENTS_INL_H
