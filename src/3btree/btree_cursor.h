/*
 * Copyright (C) 2005-2016 Christoph Rupp (chris@crupp.de).
 * All Rights Reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * See the file COPYING for License information.
 */

/*
 * btree cursors
 *
 * A Btree-Cursor is an object which is used to traverse a Btree.
 * It is a random access iterator.
 *
 * Btree-Cursors are used in Cursor structures as defined in cursor.h. But
 * some routines use them directly, mostly for performance reasons. Over
 * time these layers will be cleaned up and the separation will be improved.
 *
 * The cursor implementation is very fast. Most of the operations (i.e.
 * move previous/next) will not cause any disk access but are O(1) and
 * in-memory only. That's because a cursor is directly "coupled" to a
 * btree page (Page) that resides in memory. If the page is removed
 * from memory (i.e. because the cache decides that it needs to purge the
 * cache, or if there's a page split) then the cursor is "uncoupled", and a
 * copy of the current key is stored in the cursor. On first access, the
 * cursor is "coupled" again and basically performs a normal lookup of the key.
 *
 * The three states of a BtreeCursor("nil", "coupled", "uncoupled") can be
 * retrieved with the method get_state(), and can be modified with
 * set_to_nil(), couple_to_page() and uncouple_from_page().
 */

#ifndef UPS_BTREE_CURSORS_H
#define UPS_BTREE_CURSORS_H

#include "0root/root.h"

// Always verify that a file of level N does not include headers > N!
#include "1base/dynamic_array.h"
#include "1base/error.h"

#ifndef UPS_ROOT_H
#  error "root.h was not included"
#endif

namespace upscaledb {

struct Context;
class LocalCursor;
class BtreeIndex;
class Page;

//
// The Cursor structure for a b+tree cursor
//
class BtreeCursor
{
  public:
    enum {
      // Cursor does not point to any key
      kStateNil       = 0,
      // Cursor flag: the cursor is coupled
      kStateCoupled   = 1,
      // Cursor flag: the cursor is uncoupled
      kStateUncoupled = 2
    };

    // Constructor
    BtreeCursor(LocalCursor *parent = 0);

    // Destructor; asserts that the cursor is nil
    ~BtreeCursor() {
      ups_assert(m_state == kStateNil);
    }

    // Returns the parent cursor
    // TODO this should be private
    LocalCursor *get_parent() {
      return (m_parent);
    }

    // Clones another BtreeCursor
    void clone(BtreeCursor *other);

    // Returns the cursor's state (kStateCoupled, kStateUncoupled, kStateNil)
    uint32_t get_state() const {
      return (m_state);
    }

    // Reset's the cursor's state and uninitializes it. After this call
    // the cursor no longer points to any key.
    void set_to_nil();

    // Returns the page, index in this page and the duplicate index that this
    // cursor is coupled to. This is used by Btree functions to optimize
    // certain algorithms, i.e. when erasing the current key.
    // Asserts that the cursor is coupled.
    void get_coupled_key(Page **page, int *index = 0,
                    int *duplicate_index = 0) const {
      ups_assert(m_state == kStateCoupled);
      if (page)
        *page = m_coupled_page;
      if (index)
        *index = m_coupled_index;
      if (duplicate_index)
        *duplicate_index = m_duplicate_index;
    }

    // Returns the uncoupled key of this cursor.
    // Asserts that the cursor is uncoupled.
    ups_key_t *get_uncoupled_key() {
      ups_assert(m_state == kStateUncoupled);
      return (&m_uncoupled_key);
    }

    // Couples the cursor to a key directly in a page. Also sets the
    // duplicate index.
    void couple_to_page(Page *page, uint32_t index,
                    int duplicate_index) {
      couple_to_page(page, index);
      m_duplicate_index = duplicate_index;
    }

    // Returns the duplicate index that this cursor points to.
    int get_duplicate_index() const {
      return (m_duplicate_index);
    }

    // Sets the duplicate key we're pointing to
    void set_duplicate_index(int duplicate_index) {
      m_duplicate_index = duplicate_index;
    }

    // Uncouples the cursor
    void uncouple_from_page(Context *context);

    // Returns true if a cursor points to this btree key
    bool points_to(Context *context, Page *page, int slot);

    // Returns true if a cursor points to this external key
    bool points_to(Context *context, ups_key_t *key);

    // Moves the btree cursor to the next page
    ups_status_t move_to_next_page(Context *context);

    // Positions the cursor on a key and retrieves the record (if |record|
    // is a valid pointer)
    ups_status_t find(Context *context, ups_key_t *key, ByteArray *key_arena,
                    ups_record_t *record, ByteArray *record_arena,
                    uint32_t flags);

    // Moves the cursor to the first, last, next or previous element
    ups_status_t move(Context *context, ups_key_t *key, ByteArray *key_arena,
                    ups_record_t *record, ByteArray *record_arena,
                    uint32_t flags);

    // Returns the number of records of the referenced key
    int get_record_count(Context *context, uint32_t flags);

    // Overwrite the record of this cursor
    void overwrite(Context *context, ups_record_t *record, uint32_t flags);

    // retrieves the record size of the current record
    uint64_t get_record_size(Context *context);

    // Closes the cursor
    void close() {
      set_to_nil();
    }

    // Uncouples all cursors from a page
    // This method is called whenever the page is deleted or becomes invalid
    static void uncouple_all_cursors(Context *context, Page *page,
                    int start = 0);

  private:
    // Sets the key we're pointing to - if the cursor is coupled. Also
    // links the Cursor with |page| (and vice versa).
    void couple_to_page(Page *page, uint32_t index);

    // Removes this cursor from a page
    void remove_cursor_from_page(Page *page);

    // Couples the cursor to the current page/key
    // Asserts that the cursor is uncoupled. After this call the cursor
    // will be coupled.
    void couple(Context *context);

    // move cursor to the very first key
    ups_status_t move_first(Context *context, uint32_t flags);

    // move cursor to the very last key
    ups_status_t move_last(Context *context, uint32_t flags);

    // move cursor to the next key
    ups_status_t move_next(Context *context, uint32_t flags);

    // move cursor to the previous key
    ups_status_t move_previous(Context *context, uint32_t flags);

    // the parent cursor
    LocalCursor *m_parent;

    // The BtreeIndex instance
    BtreeIndex *m_btree;

    // "coupled" or "uncoupled" states; coupled means that the
    // cursor points into a Page object, which is in
    // memory. "uncoupled" means that the cursor has a copy
    // of the key on which it points (i.e. because the coupled page was
    // flushed to disk and removed from the cache)
    int m_state;

    // the id of the duplicate key to which this cursor is coupled
    int m_duplicate_index;

    // for coupled cursors: the page we're pointing to
    Page *m_coupled_page;

    // ... and the index of the key in that page
    int m_coupled_index;

    // for uncoupled cursors: a copy of the key at which we're pointing
    ups_key_t m_uncoupled_key;

    // a ByteArray which backs |m_uncoupled_key.data|
    ByteArray m_uncoupled_arena;

    // Linked list of cursors which point to the same page
    BtreeCursor *m_next_in_page, *m_previous_in_page;
};

} // namespace upscaledb

#endif /* UPS_BTREE_CURSORS_H */
