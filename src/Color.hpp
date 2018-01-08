#ifndef BFG_COLOR_HPP
#define BFG_COLOR_HPP

#include <roaring/roaring.hh>

#include "CompactedDBG.hpp"

class HashID : public CDBG_Data_t<HashID> {

    public:

        HashID(const uint8_t hid = 0);

        void join(const UnitigMap<HashID>& um_dest, const UnitigMap<HashID>& um_src);
        void split(const UnitigMap<HashID>& um_split, const size_t pos_split, const size_t len_split, HashID& new_data) const;

        void lock();
        inline void unlock() { __sync_and_and_fetch(&hash_id, 0x7f); }

        inline uint8_t get() const { return hash_id & 0x7f; }
        inline void set(const uint8_t hid) { hash_id = hid & 0x7f; }

    private:

        uint8_t hash_id;
};

class ColorSet {

    public:

        typedef Roaring Bitmap;

        class ColorSet_const_iterator : public std::iterator<std::forward_iterator_tag, size_t> {

            private:

                const ColorSet* cs;
                size_t color_id;

                size_t flag;
                size_t it_setBits;

                const Roaring empty_roar;
                Roaring::const_iterator it_roar;

            public:

                ColorSet_const_iterator() : cs(nullptr), flag(localBitVectorColor), it_setBits(maxBitVectorIDs), color_id(0), it_roar(empty_roar.end()) {}

                ColorSet_const_iterator(const ColorSet* cs_) : cs(cs_), it_setBits(0), color_id(0), it_roar(empty_roar.end()) {

                    flag = cs->setBits & flagMask;
                    if (flag == ptrCompressedBitmap) it_roar = cs->getConstPtrBitmap()->begin();
                }

                ColorSet_const_iterator& operator=(const ColorSet_const_iterator& o) {

                    cs = o.cs;
                    color_id = o.color_id;
                    flag = o.flag;
                    it_setBits = o.it_setBits;
                    it_roar = o.it_roar;

                    return *this;
                }

                size_t operator*() const { return color_id; }

                ColorSet_const_iterator operator++(int) {

                    ColorSet_const_iterator tmp(*this);
                    operator++();
                    return tmp;
                }

                ColorSet_const_iterator& operator++() {

                    if (flag == ptrCompressedBitmap) {

                        if (it_roar != cs->getConstPtrBitmap()->end()) color_id = *(++it_roar);
                    }
                    else if (flag == localBitVectorColor){

                        while (it_setBits < maxBitVectorIDs){

                            if (((cs->setBits >> (it_setBits + 2)) & 0x1) != 0){

                                color_id = it_setBits;
                                break;
                            }

                            ++it_setBits;
                        }
                    }
                    else if ((flag == localSingleColor) && (it_setBits < 1)){

                        color_id = cs->setBits >> 2;
                        ++it_setBits;
                    }

                    return *this;
                }

                bool operator==(const ColorSet_const_iterator& o) {

                    return  (cs == o.cs) && (color_id == o.color_id) && (flag == o.flag) &&
                            ((flag == ptrCompressedBitmap) ? (it_roar == o.it_roar) : (it_setBits == o.it_setBits));
                }

                bool operator!=(const ColorSet_const_iterator& o) {

                    return  (cs != o.cs) || (color_id != o.color_id) || (flag != o.flag) ||
                            ((flag == ptrCompressedBitmap) ? (it_roar != o.it_roar) : (it_setBits != o.it_setBits));
                }
        };

        typedef ColorSet_const_iterator const_iterator;

        ColorSet();
        ~ColorSet();

        void add(const UnitigMap<HashID>& um, const size_t color_id);
        bool contains(const UnitigMap<HashID>& um, const size_t color_id) const;

        size_t size() const;

        const_iterator begin() const {

            const_iterator it(this);
            return ++it;
        }

        const_iterator end() const { return const_iterator(); }

    private:

        void releasePointer();

        inline Bitmap* getPtrBitmap() const { return reinterpret_cast<Bitmap*>(setBits & pointerMask); }
        inline const Bitmap* getConstPtrBitmap() const { return reinterpret_cast<const Bitmap*>(setBits & pointerMask); }

        static const size_t maxBitVectorIDs = 62; // 64 bits - 2 bits for the color set type

        // asBits and asPointer represent:
        // Flag 0 - A pointer to a compressed bitmap containing colors
        // Flag 1 - A bit vector of 62 bits storing presence/absence of up to 62 colors (one bit = one color)
        // Flag 2 - A single integer which is a color
        // Flag 3 - Unused

        static const uintptr_t ptrCompressedBitmap = 0x0;
        static const uintptr_t localBitVectorColor = 0x1;
        static const uintptr_t localSingleColor = 0x2;

        static const uintptr_t flagMask = 0x3;
        static const uintptr_t pointerMask = 0xfffffffffffffffc;

        union {

            uintptr_t setBits;
            Bitmap* setPointer;
        };
};

#endif
