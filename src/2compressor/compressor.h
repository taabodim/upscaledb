/*
 * Copyright (C) 2005-2014 Christoph Rupp (chris@crupp.de).
 * All Rights Reserved.
 *
 * NOTICE: All information contained herein is, and remains the property
 * of Christoph Rupp and his suppliers, if any. The intellectual and
 * technical concepts contained herein are proprietary to Christoph Rupp
 * and his suppliers and may be covered by Patents, patents in process,
 * and are protected by trade secret or copyright law. Dissemination of
 * this information or reproduction of this material is strictly forbidden
 * unless prior written permission is obtained from Christoph Rupp.
 *
 * See the file COPYING for License information.
 */

/*
 * An abstract base class for a compressor.
 *
 * @exception_safe: strong
 * @thread_safe: no
 */

#ifndef HAM_COMPRESSOR_H
#define HAM_COMPRESSOR_H

#include "0root/root.h"

// Always verify that a file of level N does not include headers > N!
#include "1base/byte_array.h"

#ifndef HAM_ROOT_H
#  error "root.h was not included"
#endif

namespace hamsterdb {

class Compressor {
  public:
    // Constructor
    Compressor()
      : m_skip(0) {
    }

    // Virtual destructor - can be overwritten
    virtual ~Compressor() {
    }

    // Compresses |inlength1| bytes of data in |inp1|. If |inp2| is supplied
    // then |inp2| will be compressed immediately after |inp1|.
    // The compressed data can then be retrieved with |get_output_data()|.
    //
    // Returns the length of the compressed data.
    virtual uint32_t compress(const uint8_t *inp1, uint32_t inlength1,
                    const uint8_t *inp2 = 0, uint32_t inlength2 = 0) = 0;

    // Reserves |n| bytes in the output buffer; can be used by the caller
    // to insert flags or sizes
    void reserve(int n) {
      m_skip = n;
    }

    // Decompresses |inlength| bytes of data in |inp|. |outlength| is the
    // expected size of the decompressed data.
    virtual void decompress(const uint8_t *inp, uint32_t inlength,
                    uint32_t outlength) = 0;

    // Decompresses |inlength| bytes of data in |inp|. |outlength| is the
    // expected size of the decompressed data. Uses the caller's |arena|
    // for storage.
    virtual void decompress(const uint8_t *inp, uint32_t inlength,
                    uint32_t outlength, ByteArray *arena) = 0;

    // Decompresses |inlength| bytes of data in |inp|. |outlength| is the
    // expected size of the decompressed data. Uses the caller's |destination|
    // for storage.
    virtual void decompress(const uint8_t *inp, uint32_t inlength,
                    uint32_t outlength, uint8_t *destination) = 0;

    // Retrieves the compressed (or decompressed) data, including its size
    const uint8_t *get_output_data() const {
      return ((uint8_t *)m_arena.get_ptr());
    }

    // Same as above, but non-const
    uint8_t *get_output_data() {
      return ((uint8_t *)m_arena.get_ptr());
    }

    // Returns the internal memory arena
    ByteArray *get_arena() {
      return (&m_arena);
    }

  protected:
    // The ByteArray which stores the compressed (or decompressed) data
    ByteArray m_arena;

    // Number of bytes to reserve for the caller
    int m_skip;
};

}; // namespace hamsterdb

#endif // HAM_COMPRESSOR_H
