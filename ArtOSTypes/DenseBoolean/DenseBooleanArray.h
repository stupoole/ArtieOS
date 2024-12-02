//
// Created by artypoole on 02/12/24.
//

#ifndef DENSEBOOLEANARRAY_H
#define DENSEBOOLEANARRAY_H
#include "DenseBoolean.h"

inline size_t DBA_ERR_IDX = -1;

template <typename int_like>
class DenseBooleanArray
{
public:
    explicit DenseBooleanArray(const size_t total_bits)
    {
        capacity = total_bits;
        n_bits = sizeof(int_like) * 8;
        array_len = (total_bits + (n_bits - 1)) / n_bits;
        array = new DenseBoolean<int_like>[array_len];
    }

    explicit DenseBooleanArray(const size_t total_bits, const bool def_bool)
    {
        capacity = total_bits;
        n_bits = sizeof(int_like) * 8;
        array_len = (total_bits + (n_bits - 1)) / n_bits;
        array = new DenseBoolean<int_like>[array_len];
        int_like v = 0;
        if (def_bool) v = ~0;
        for (int i = 0; i < array_len; i++)
        {
            array[i].set_data(v);
        }
    }

    ~DenseBooleanArray()
    {
        delete[] array;
    }


    bool operator [](const size_t bit_idx)
    {
        return array[bit_idx / n_bits].data() & (1 << (bit_idx % n_bits));
    }

    void set_bit(const size_t idx, bool b)
    {
        array[idx / n_bits].set_bit(idx % n_bits, b);
    }

    int get_next_false()
    {
        return get_next_false(0);
    }

    // Exact same logic as get_next_true but all the data is inverted.
    size_t get_next_false(const size_t offset)
    {
        size_t array_idx = offset / n_bits;
        size_t item_idx = offset % n_bits;
        int_like mask = (-1) << item_idx; // returns, e.g. 11111000 if item_idx == 3
        int_like data = ~array[array_idx].data() & mask; // ignores lowest bits
        item_idx = 0;

        while (data == 0 && array_idx < array_len) // while data is all ones (no falses in int_like)
        {
            array_idx++;
            data = ~array[array_idx].data();
        }

        while (!(data & 0x1 << item_idx) && item_idx < n_bits) // bit wise
        {
            item_idx++;
        }

        item_idx = array_idx * n_bits + item_idx;
        if (item_idx >= capacity) return DBA_ERR_IDX; // possible if last entry is not full
        return item_idx;
    }



    size_t get_next_falses(const size_t offset, size_t n)
    {
        size_t start_idx = get_next_false(offset);
        size_t next_idx = get_next_true(start_idx);
        while (next_idx - start_idx < n && start_idx < DBA_ERR_IDX && next_idx < DBA_ERR_IDX)
        {
            start_idx = get_next_false(next_idx);
            next_idx = get_next_true(start_idx);
        }
        if (start_idx == DBA_ERR_IDX) return DBA_ERR_IDX; // no trues after offset
        if (next_idx == DBA_ERR_IDX  && start_idx + n > capacity) return DBA_ERR_IDX; // doesn't fit
        return start_idx; // should fit
    }

    size_t get_next_true()
    {
        return get_next_true(0);
    }


    size_t get_next_true(const size_t offset)
    {
        size_t array_idx = offset / n_bits;
        size_t item_idx = offset % n_bits;
        int_like mask = (-1) << item_idx;
        int_like data = array[array_idx].data() & mask; // ignores lowest bits
        item_idx = 0;
        while (data == 0 && array_idx < array_len) // while data is all zeros (no trues in int_like)
        {
            array_idx++;
            data = array[array_idx].data();
        }

        while (!(data & 0x1 << item_idx) && item_idx < n_bits) // bitwise search within.
        {
            item_idx++;
        }

        item_idx = array_idx * n_bits + item_idx;
        if (item_idx >= capacity) return DBA_ERR_IDX; // possible if last entry is not full
        return item_idx;
    }

    // gets start idx of a contiguous chunk of trues with length at least as large as n.
    size_t get_next_trues(const size_t offset, size_t n)
    {
        size_t start_idx = get_next_true(offset);
        size_t next_idx = get_next_false(start_idx);
        while (next_idx - start_idx < n && start_idx < DBA_ERR_IDX && next_idx < DBA_ERR_IDX)
        {
            start_idx = get_next_true(next_idx);
            next_idx = get_next_false(start_idx);
        }
        if (start_idx == DBA_ERR_IDX) return DBA_ERR_IDX; // no trues after offset
        if (next_idx == DBA_ERR_IDX && start_idx + n > capacity) return DBA_ERR_IDX; // doesn't fit
        return start_idx; // should fit
    }

    size_t get_array_len() { return array_len; }

    size_t set_range(size_t start, size_t n, bool b) { return DBA_ERR_IDX; }

private:
    DenseBoolean<int_like>* array;
    size_t array_len; // n_items in array
    size_t capacity; // bits
    size_t n_bits;
};


#endif //DENSEBOOLEANARRAY_H
