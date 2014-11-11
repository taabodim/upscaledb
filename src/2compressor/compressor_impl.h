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
 * A parameterized implementation for the abstract Compressor class.
 *
 * @exception_safe: strong
 * @thread_safe: no
 */

#ifndef HAM_COMPRESSOR_IMPL_H
#define HAM_COMPRESSOR_IMPL_H

#include "0root/root.h"

// Always verify that a file of level N does not include headers > N!
#include "2compressor/compressor.h"

#ifndef HAM_ROOT_H
#  error "root.h was not included"
#endif

namespace hamsterdb {

template<class T>
class CompressorImpl : public Compressor {
  public:
    // Compresses |inlength1| bytes of data in |inp1|. If |inp2| is supplied
    // then |inp2| will be compressed immediately after |inp1|.
    // The compressed data can then be retrieved with |get_output_data()|.
    //
    // Returns the length of the compressed data.
    virtual uint32_t compress(const uint8_t *inp1, uint32_t inlength1,
                    const uint8_t *inp2 = 0, uint32_t inlength2 = 0) {
      uint32_t clen = 0;
      uint32_t arena_size = m_skip + m_impl.get_compressed_length(inlength1);
      if (inp2 != 0)
        arena_size += m_impl.get_compressed_length(inlength2);
      m_arena.resize(arena_size + m_skip);

      uint8_t *out = (uint8_t *)m_arena.get_ptr() + m_skip;

      clen = m_impl.compress(inp1, inlength1, out,
                      m_arena.get_size() - m_skip);
      if (inp2)
        clen += m_impl.compress(inp2, inlength2, out + clen,
                        m_arena.get_size() - clen - m_skip);
      return (clen);
    }

    // Decompresses |inlength| bytes of data in |inp|. |outlength| is the
    // expected size of the decompressed data.
    virtual void decompress(const uint8_t *inp, uint32_t inlength,
                    uint32_t outlength) {
      m_arena.resize(outlength);
      m_impl.decompress(inp, inlength, (uint8_t *)m_arena.get_ptr(), outlength);
    }

    // Decompresses |inlength| bytes of data in |inp|. |outlength| is the
    // expected size of the decompressed data. Uses the caller's |arena|
    // for storage.
    virtual void decompress(const uint8_t *inp, uint32_t inlength,
                    uint32_t outlength, ByteArray *arena) {
      arena->resize(outlength);
      m_impl.decompress(inp, inlength, (uint8_t *)arena->get_ptr(), outlength);
    }

    // Decompresses |inlength| bytes of data in |inp|. |outlength| is the
    // expected size of the decompressed data. Uses the caller's |destination|
    // for storage.
    virtual void decompress(const uint8_t *inp, uint32_t inlength,
                    uint32_t outlength, uint8_t *destination) {
      m_impl.decompress(inp, inlength, destination, outlength);
    }

  private:
    // The actual implementation
    T m_impl;
};

}; // namespace hamsterdb

#endif // HAM_COMPRESSOR_IMPL_H
