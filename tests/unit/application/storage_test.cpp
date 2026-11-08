#include "redis/storage/InMemoryKeyValueStore.h"

#include <cassert>
#include <iostream>

int main() {
  redis::storage::InMemoryKeyValueStore store;

  assert(!store.get("name").has_value());
  assert(store.set("name", "Alice"));
  
  auto value = store.get("name");
  assert(value.has_value());
  assert(*value == "Alice");
  
  assert(store.set("name", "Bob"));

  value = store.get("name");
  assert(value.has_value());
  assert(*value == "Bob");

  assert(store.del("name"));
  assert(!store.get("name").has_value());

  // Deleting a missing key should return false.
  assert(!store.del("missing"));
  
  std::cout << "Storage tests passed\n";
}
