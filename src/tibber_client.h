#pragma once

#include <Arduino.h>
#include "price_store.h"

/// Tibber GraphQL API client.
namespace TibberClient {

/// Initialize the client with the API token and home index.
void init(const char* token, uint8_t homeIndex = 0);

/// Fetch today's and tomorrow's prices from the Tibber API.
/// Populates PriceStore on success.
/// @return true on success.
bool fetchPrices();

/// @return HTTP status code from the last request, or 0 if no request made.
int getLastHttpCode();

/// @return Human-readable error message from the last failed request.
const char* getLastError();

}  // namespace TibberClient

