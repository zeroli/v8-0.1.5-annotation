// Copyright 2008 Google Inc. All Rights Reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//     * Neither the name of Google Inc. nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "v8.h"

#include "hashmap.h"

namespace v8 { namespace internal {


static inline bool IsPowerOf2(uint32_t x) {
  ASSERT(x != 0);
  return (x & (x - 1)) == 0;
}


Allocator HashMap::DefaultAllocator;


HashMap::HashMap() {
  allocator_ = NULL;
  match_ = NULL;
}


HashMap::HashMap(MatchFun match,
                 Allocator* allocator,
                 uint32_t initial_capacity) {
  allocator_ = allocator;
  match_ = match;
  Initialize(initial_capacity);
}


HashMap::~HashMap() {
  if (allocator_) {
    allocator_->Delete(map_);
  }
}


HashMap::Entry* HashMap::Lookup(void* key, uint32_t hash, bool insert) {
  // Find a matching entry.
  Entry* p = Probe(key, hash);
  if (p->key != NULL) {
    return p;
  }

  // No entry found; insert one if necessary.
  if (insert) {
    p->key = key;
    p->value = NULL;
    p->hash = hash;
    occupancy_++;

    // Grow the map if we reached >= 80% occupancy.
    if (occupancy_ + occupancy_/4 >= capacity_) {
      Resize();
      p = Probe(key, hash);
    }

    return p;
  }

  // No entry found and none inserted.
  return NULL;
}


void HashMap::Clear() {
  // Mark all entries as empty.
  const Entry* end = map_end();
  for (Entry* p = map_; p < end; p++) {
    p->key = NULL;
  }
  occupancy_ = 0;
}


HashMap::Entry* HashMap::Start() const {
  return Next(map_ - 1);
}


HashMap::Entry* HashMap::Next(Entry* p) const {
  const Entry* end = map_end();
  ASSERT(map_ - 1 <= p && p < end);
  for (p++; p < end; p++) {
    if (p->key != NULL) {
      return p;
    }
  }
  return NULL;
}


HashMap::Entry* HashMap::Probe(void* key, uint32_t hash) {
  ASSERT(key != NULL);

  ASSERT(IsPowerOf2(capacity_));
  Entry* p = map_ + (hash & (capacity_ - 1));
  const Entry* end = map_end();
  ASSERT(map_ <= p && p < end);

  ASSERT(occupancy_ < capacity_);  // guarantees loop termination
  while (p->key != NULL && (hash != p->hash || !match_(key, p->key))) {
    p++;
    if (p >= end) {
      p = map_;
    }
  }

  return p;
}


void HashMap::Initialize(uint32_t capacity) {
  ASSERT(IsPowerOf2(capacity));
  map_ = reinterpret_cast<Entry*>(allocator_->New(capacity * sizeof(Entry)));
  if (map_ == NULL) V8::FatalProcessOutOfMemory("HashMap::Initialize");
  capacity_ = capacity;
  Clear();
}


void HashMap::Resize() {
  Entry* map = map_;
  uint32_t n = occupancy_;

  // Allocate larger map.
  Initialize(capacity_ * 2);

  // Rehash all current entries.
  for (Entry* p = map; n > 0; p++) {
    if (p->key != NULL) {
      Lookup(p->key, p->hash, true)->value = p->value;
      n--;
    }
  }

  // Delete old map.
  allocator_->Delete(map);
}


} }  // namespace v8::internal
