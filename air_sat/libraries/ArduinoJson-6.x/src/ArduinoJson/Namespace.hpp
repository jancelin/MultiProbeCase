// ArduinoJson - arduinojson.org
// Copyright Benoit Blanchon 2014-2020
// MIT License

#pragma once

#include <ArduinoJson/Configuration.hpp>
#include <ArduinoJson/version.hpp>

#ifndef ARDUINOJSON_NAMESPACE

#define ARDUINOJSON_DO_CONCAT(A, B) A##B
#define ARDUINOJSON_CONCAT2(A, B) ARDUINOJSON_DO_CONCAT(A, B)
#define ARDUINOJSON_CONCAT4(A, B, C, D) \
  ARDUINOJSON_CONCAT2(ARDUINOJSON_CONCAT2(A, B), ARDUINOJSON_CONCAT2(C, D))
#define ARDUINOJSON_CONCAT8(A, B, C, D, E, F, G, H)    \
  ARDUINOJSON_CONCAT2(ARDUINOJSON_CONCAT4(A, B, C, D), \
                      ARDUINOJSON_CONCAT4(E, F, G, H))
#define ARDUINOJSON_CONCAT13(A, B, C, D, E, F, G, H, I, J, K, L, M)   \
  ARDUINOJSON_CONCAT8(A, B, C, D, E, ARDUINOJSON_CONCAT4(F, G, H, I), \
                      ARDUINOJSON_CONCAT2(J, K), ARDUINOJSON_CONCAT2(L, M))

#define ARDUINOJSON_NAMESPACE                                            \
  ARDUINOJSON_CONCAT13(                                                  \
      ArduinoJson, ARDUINOJSON_VERSION_MAJOR, ARDUINOJSON_VERSION_MINOR, \
      ARDUINOJSON_VERSION_REVISION, _, ARDUINOJSON_USE_LONG_LONG,        \
      ARDUINOJSON_USE_DOUBLE, ARDUINOJSON_DECODE_UNICODE,                \
      ARDUINOJSON_ENABLE_NAN, ARDUINOJSON_ENABLE_INFINITY,               \
      ARDUINOJSON_ENABLE_PROGMEM, ARDUINOJSON_ENABLE_COMMENTS,           \
      ARDUINOJSON_ENABLE_STRING_DEDUPLICATION)

#endif
