/*  GRAPHITE2 LICENSING

    Copyright 2010, SIL International
    All rights reserved.

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation; either version 2.1 of License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should also have received a copy of the GNU Lesser General Public
    License along with this library in the file named "LICENSE".
    If not, write to the Free Software Foundation, 51 Franklin Street, 
    Suite 500, Boston, MA 02110-1335, USA or visit their web page on the 
    internet at http://www.fsf.org/licenses/lgpl.html.

Alternatively, the contents of this file may be used under the terms of the
Mozilla Public License (http://mozilla.org/MPL) or the GNU General Public
License, as published by the Free Software Foundation, either version 2
of the License or (at your option) any later version.
*/
#pragma once

#include "inc/Main.h"

#include <cassert>

#include "inc/CharInfo.h"
#include "inc/Face.h"
#include "inc/FeatureVal.h"
#include "inc/GlyphCache.h"
#include "inc/GlyphFace.h"
#include "inc/Slot.h"
#include "inc/Position.h"
#include "inc/List.h"
#include "inc/Collider.h"

#define MAX_SEG_GROWTH_FACTOR  256

namespace graphite2 {

typedef Vector<Features>        FeatureList;
typedef Vector<Slot *>          SlotRope;
typedef Vector<int16 *>         AttributeRope;
typedef Vector<SlotJustify *>   JustifyRope;

#ifndef GRAPHITE2_NSEGCACHE
class SegmentScopeState;
#endif
class Font;
class Segment;
class Silf;

enum SpliceParam {
/** sub-Segments longer than this are not cached
 * (in Unicode code points) */
    eMaxSpliceSize = 96
};

enum justFlags {
    gr_justStartInline = 1,
    gr_justEndInline = 2
};

class SegmentScopeState
{
private:
    friend class Segment;
    Slot * realFirstSlot;
    Slot * slotBeforeScope;
    Slot * slotAfterScope;
    Slot * realLastSlot;
    size_t numGlyphsOutsideScope;
};

class Segment
{
    // Prevent copying of any kind.
    Segment(const Segment&);
    Segment& operator=(const Segment&);

public:

    enum {
        SEG_INITCOLLISIONS = 1
    };

    unsigned int slotCount() const { return m_numGlyphs; }      //one slot per glyph
    void extendLength(int num) { m_numGlyphs += num; }
    Position advance() const { return m_advance; }
    bool runGraphite() { if (m_silf) return m_face->runGraphite(this, m_silf); else return true;};
    void chooseSilf(uint32 script) { m_silf = m_face->chooseSilf(script); }
    const Silf *silf() const { return m_silf; }
    unsigned int charInfoCount() const { return m_numCharinfo; }
    const CharInfo *charinfo(unsigned int index) const { return index < m_numCharinfo ? m_charinfo + index : NULL; }
    CharInfo *charinfo(unsigned int index) { return index < m_numCharinfo ? m_charinfo + index : NULL; }

    Segment(unsigned int numchars, const Face* face, uint32 script, int dir);
    ~Segment();
#ifndef GRAPHITE2_NSEGCACHE
    SegmentScopeState setScope(Slot * firstSlot, Slot * lastSlot, size_t subLength);
    void removeScope(SegmentScopeState & state);
    void append(const Segment &other);
    void splice(size_t offset, size_t length, Slot * const startSlot,
            Slot * endSlot, const Slot * srcSlot,
            const size_t numGlyphs);
#endif
    uint8 flags() const { return m_flags; }
    void flags(uint8 f) { m_flags = f; }
    Slot *first() { return m_first; }
    void first(Slot *p) { m_first = p; }
    Slot *last() { return m_last; }
    void last(Slot *p) { m_last = p; }
    void appendSlot(int i, int cid, int gid, int fid, size_t coffset);
    Slot *newSlot();
    void freeSlot(Slot *);
    SlotJustify *newJustify();
    void freeJustify(SlotJustify *aJustify);
    Position positionSlots(const Font *font=0, Slot *first=0, Slot *last=0, bool isRtl = false, bool isFinal = true);
    void associateChars(int offset, int num);
    void linkClusters(Slot *first, Slot *last);
    uint16 getClassGlyph(uint16 cid, uint16 offset) const { return m_silf->getClassGlyph(cid, offset); }
    uint16 findClassIndex(uint16 cid, uint16 gid) const { return m_silf->findClassIndex(cid, gid); }
    int addFeatures(const Features& feats) { m_feats.push_back(feats); return m_feats.size() - 1; }
    uint32 getFeature(int index, uint8 findex) const { const FeatureRef* pFR=m_face->theSill().theFeatureMap().featureRef(findex); if (!pFR) return 0; else return pFR->getFeatureVal(m_feats[index]); }
    void setFeature(int index, uint8 findex, uint32 val) {
        const FeatureRef* pFR=m_face->theSill().theFeatureMap().featureRef(findex); 
        if (pFR)
        {
            if (val > pFR->maxVal()) val = pFR->maxVal();
            pFR->applyValToFeature(val, m_feats[index]);
        } }
    int8 dir() const { return m_dir; }
    void dir(int8 val) { m_dir = val; }
    bool currdir() const { return ((m_dir >> 6) ^ m_dir) & 1; }
    unsigned int passBits() const { return m_passBits; }
    void mergePassBits(const unsigned int val) { m_passBits &= val; }
    int16 glyphAttr(uint16 gid, uint16 gattr) const { const GlyphFace * p = m_face->glyphs().glyphSafe(gid); return p ? p->attrs()[gattr] : 0; }
    int32 getGlyphMetric(Slot *iSlot, uint8 metric, uint8 attrLevel, bool rtl) const;
    float glyphAdvance(uint16 gid) const { return m_face->glyphs().glyph(gid)->theAdvance().x; }
    const Rect &theGlyphBBoxTemporary(uint16 gid) const { return m_face->glyphs().glyph(gid)->theBBox(); }   //warning value may become invalid when another glyph is accessed
    Slot *findRoot(Slot *is) const { return is->attachedTo() ? findRoot(is->attachedTo()) : is; }
    int numAttrs() const { return m_silf->numUser(); }
    int defaultOriginal() const { return m_defaultOriginal; }
    const Face * getFace() const { return m_face; }
    const Features & getFeatures(unsigned int /*charIndex*/) { assert(m_feats.size() == 1); return m_feats[0]; }
    void bidiPass(int paradir, uint8 aMirror);
    int8 getSlotBidiClass(Slot *s) const;
    void doMirror(uint16 aMirror);
    Slot *addLineEnd(Slot *nSlot);
    void delLineEnd(Slot *s);
    bool hasJustification() const { return m_justifies.size() != 0; }
    void reverseSlots();

    bool isWhitespace(const int cid) const;
    bool hasCollisionInfo() const { return m_collisions != 0; }
    SlotCollision *collisionInfo(const Slot *s) const { return m_collisions ? m_collisions + s->index() : NULL; }

    CLASS_NEW_DELETE

public:       //only used by: GrSegment* makeAndInitialize(const GrFont *font, const GrFace *face, uint32 script, const FeaturesHandle& pFeats/*must not be IsNull*/, encform enc, const void* pStart, size_t nChars, int dir);
    bool read_text(const Face *face, const Features* pFeats/*must not be NULL*/, gr_encform enc, const void*pStart, size_t nChars);
    void finalise(const Font *font, bool reverse=false);
    float justify(Slot *pSlot, const Font *font, float width, enum justFlags flags, Slot *pFirst, Slot *pLast);
    bool initCollisions();
  
private:
    Position        m_advance;          // whole segment advance
    SlotRope        m_slots;            // Vector of slot buffers
    AttributeRope   m_userAttrs;        // Vector of userAttrs buffers
    JustifyRope     m_justifies;        // Slot justification info buffers
    FeatureList     m_feats;            // feature settings referenced by charinfos in this segment
    Slot          * m_freeSlots;        // linked list of free slots
    SlotJustify   * m_freeJustifies;    // Slot justification blocks free list
    CharInfo      * m_charinfo;         // character info, one per input character
    SlotCollision * m_collisions;       // Array of SlotCollisions for each slot
    const Face    * m_face;             // GrFace
    const Silf    * m_silf;
    Slot          * m_first;            // first slot in segment
    Slot          * m_last;             // last slot in segment
    unsigned int    m_bufSize,          // how big a buffer to create when need more slots
                    m_numGlyphs,
                    m_numCharinfo,      // size of the array and number of input characters
                    m_passBits;         // if bit set then skip pass
    int             m_defaultOriginal;  // number of whitespace chars in the string
    int8            m_dir;
    uint8           m_flags;            // General purpose flags
};

inline
int8 Segment::getSlotBidiClass(Slot *s) const
{
    int8 res = s->getBidiClass();
    if (res != -1) return res;
    res = glyphAttr(s->gid(), m_silf->aBidi());
    s->setBidiClass(res);
    return res;
}

inline
void Segment::finalise(const Font *font, bool reverse)
{
    if (!m_first) return;

    m_advance = positionSlots(font, m_first, m_last, m_silf->dir(), true);
    //associateChars(0, m_numCharinfo);
    if (reverse && currdir() != (m_dir & 1))
        reverseSlots();
    linkClusters(m_first, m_last);
}

inline
int32 Segment::getGlyphMetric(Slot *iSlot, uint8 metric, uint8 attrLevel, bool rtl) const {
    if (attrLevel > 0)
    {
        Slot *is = findRoot(iSlot);
        return is->clusterMetric(this, metric, attrLevel, rtl);
    }
    else
        return m_face->getGlyphMetric(iSlot->gid(), metric);
}

inline
bool Segment::isWhitespace(const int cid) const
{
    return ((cid >= 0x0009) * (cid <= 0x000D)
         + (cid == 0x0020)
         + (cid == 0x0085)
         + (cid == 0x00A0)
         + (cid == 0x1680)
         + (cid == 0x180E)
         + (cid >= 0x2000) * (cid <= 0x200A)
         + (cid == 0x2028)
         + (cid == 0x2029)
         + (cid == 0x202F)
         + (cid == 0x205F)
         + (cid == 0x3000)) != 0;
}

} // namespace graphite2

struct gr_segment : public graphite2::Segment {};

