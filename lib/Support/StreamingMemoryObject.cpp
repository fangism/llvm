//===- StreamingMemoryObject.cpp - Streamable data interface -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/Support/StreamingMemoryObject.h"
#include "llvm/Support/Compiler.h"
#include <cassert>
#include <cstddef>
#include <cstring>


using namespace llvm;

namespace {

class RawMemoryObject : public MemoryObject {
public:
  RawMemoryObject(const unsigned char *Start, const unsigned char *End) :
    FirstChar(Start), LastChar(End) {
    assert(LastChar >= FirstChar && "Invalid start/end range");
  }

  uint64_t getExtent() const override {
    return LastChar - FirstChar;
  }
  uint64_t readBytes(uint8_t *Buf, uint64_t Size,
                     uint64_t Address) const override;
  const uint8_t *getPointer(uint64_t address, uint64_t size) const override;
  bool isValidAddress(uint64_t address) const override {
    return validAddress(address);
  }
  bool isObjectEnd(uint64_t address) const override {
    return objectEnd(address);
  }

private:
  const uint8_t* const FirstChar;
  const uint8_t* const LastChar;

  // These are implemented as inline functions here to avoid multiple virtual
  // calls per public function
  bool validAddress(uint64_t address) const {
    return static_cast<std::ptrdiff_t>(address) < LastChar - FirstChar;
  }
  bool objectEnd(uint64_t address) const {
    return static_cast<std::ptrdiff_t>(address) == LastChar - FirstChar;
  }

  RawMemoryObject(const RawMemoryObject&) LLVM_DELETED_FUNCTION;
  void operator=(const RawMemoryObject&) LLVM_DELETED_FUNCTION;
};

uint64_t RawMemoryObject::readBytes(uint8_t *Buf, uint64_t Size,
                                    uint64_t Address) const {
  uint64_t BufferSize = LastChar - FirstChar;
  if (Address >= BufferSize)
    return 0;

  uint64_t End = Address + Size;
  if (End > BufferSize)
    End = BufferSize;

  Size = End - Address;
  assert(Size >= 0);
  memcpy(Buf, (uint8_t *)(Address + FirstChar), Size);
  return Size;
}

const uint8_t *RawMemoryObject::getPointer(uint64_t address,
                                           uint64_t size) const {
  return FirstChar + address;
}
} // anonymous namespace

namespace llvm {
// If the bitcode has a header, then its size is known, and we don't have to
// block until we actually want to read it.
bool StreamingMemoryObject::isValidAddress(uint64_t address) const {
  if (ObjectSize && address < ObjectSize) return true;
    return fetchToPos(address);
}

bool StreamingMemoryObject::isObjectEnd(uint64_t address) const {
  if (ObjectSize) return address == ObjectSize;
  fetchToPos(address);
  return address == ObjectSize && address != 0;
}

uint64_t StreamingMemoryObject::getExtent() const {
  if (ObjectSize) return ObjectSize;
  size_t pos = BytesRead + kChunkSize;
  // keep fetching until we run out of bytes
  while (fetchToPos(pos)) pos += kChunkSize;
  return ObjectSize;
}

uint64_t StreamingMemoryObject::readBytes(uint8_t *Buf, uint64_t Size,
                                          uint64_t Address) const {
  fetchToPos(Address + Size - 1);
  uint64_t BufferSize = Bytes.size() - BytesSkipped;
  if (Address >= BufferSize)
    return 0;

  uint64_t End = Address + Size;
  if (End > BufferSize)
    End = BufferSize;
  Size = End - Address;
  assert(Size >= 0);
  memcpy(Buf, &Bytes[Address + BytesSkipped], Size);
  return Size;
}

bool StreamingMemoryObject::dropLeadingBytes(size_t s) {
  if (BytesRead < s) return true;
  BytesSkipped = s;
  BytesRead -= s;
  return false;
}

void StreamingMemoryObject::setKnownObjectSize(size_t size) {
  ObjectSize = size;
  Bytes.reserve(size);
}

MemoryObject *getNonStreamedMemoryObject(const unsigned char *Start,
                                         const unsigned char *End) {
  return new RawMemoryObject(Start, End);
}

StreamingMemoryObject::StreamingMemoryObject(DataStreamer *streamer) :
  Bytes(kChunkSize), Streamer(streamer), BytesRead(0), BytesSkipped(0),
  ObjectSize(0), EOFReached(false) {
  BytesRead = streamer->GetBytes(&Bytes[0], kChunkSize);
}
}
