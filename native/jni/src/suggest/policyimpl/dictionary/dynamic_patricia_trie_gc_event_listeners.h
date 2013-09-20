/*
 * Copyright (C) 2013, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef LATINIME_DYNAMIC_PATRICIA_TRIE_GC_EVENT_LISTENERS_H
#define LATINIME_DYNAMIC_PATRICIA_TRIE_GC_EVENT_LISTENERS_H

#include <vector>

#include "defines.h"
#include "suggest/policyimpl/dictionary/bigram/dynamic_bigram_list_policy.h"
#include "suggest/policyimpl/dictionary/dynamic_patricia_trie_reading_helper.h"
#include "suggest/policyimpl/dictionary/dynamic_patricia_trie_writing_helper.h"
#include "suggest/policyimpl/dictionary/dynamic_patricia_trie_writing_utils.h"
#include "suggest/policyimpl/dictionary/utils/buffer_with_extendable_buffer.h"
#include "utils/hash_map_compat.h"

namespace latinime {

class DynamicPatriciaTrieGcEventListeners {
 public:
    // Updates all PtNodes that can be reached from the root. Checks if each PtNode is useless or
    // not and marks useless PtNodes as deleted. Such deleted PtNodes will be discarded in the GC.
    // TODO: Concatenate non-terminal PtNodes.
    class ListenerForUpdatingUnigramProbabilityAndMarkingUselessPtNodesAsDeleted
        : public DynamicPatriciaTrieReadingHelper::TraversingEventListener {
     public:
        ListenerForUpdatingUnigramProbabilityAndMarkingUselessPtNodesAsDeleted(
                DynamicPatriciaTrieWritingHelper *const writingHelper,
                BufferWithExtendableBuffer *const buffer)
                : mWritingHelper(writingHelper), mBuffer(buffer), valueStack(),
                  mChildrenValue(0) {}

        ~ListenerForUpdatingUnigramProbabilityAndMarkingUselessPtNodesAsDeleted() {};

        bool onAscend() {
            if (valueStack.empty()) {
                return false;
            }
            mChildrenValue = valueStack.back();
            valueStack.pop_back();
            return true;
        }

        bool onDescend(const int ptNodeArrayPos) {
            valueStack.push_back(0);
            return true;
        }

        bool onReadingPtNodeArrayTail() { return true; }

        bool onVisitingPtNode(const DynamicPatriciaTrieNodeReader *const node,
                const int *const nodeCodePoints);

     private:
        DISALLOW_IMPLICIT_CONSTRUCTORS(
                ListenerForUpdatingUnigramProbabilityAndMarkingUselessPtNodesAsDeleted);

        DynamicPatriciaTrieWritingHelper *const mWritingHelper;
        BufferWithExtendableBuffer *const mBuffer;
        std::vector<int> valueStack;
        int mChildrenValue;
    };

    // Updates all bigram entries that are held by valid PtNodes. This removes useless bigram
    // entries.
    class ListenerForUpdatingBigramProbability
            : public DynamicPatriciaTrieReadingHelper::TraversingEventListener {
     public:
        ListenerForUpdatingBigramProbability(DynamicBigramListPolicy *const bigramPolicy)
                : mBigramPolicy(bigramPolicy) {}

        bool onAscend() { return true; }

        bool onDescend(const int ptNodeArrayPos) { return true; }

        bool onReadingPtNodeArrayTail() { return true; }

        bool onVisitingPtNode(const DynamicPatriciaTrieNodeReader *const node,
                const int *const nodeCodePoints) {
            if (!node->isDeleted()) {
                int pos = node->getBigramsPos();
                if (pos != NOT_A_DICT_POS) {
                    if (!mBigramPolicy->updateAllBigramEntriesAndDeleteUselessEntries(&pos)) {
                        return false;
                    }
                }
            }
            return true;
        }

     private:
        DISALLOW_IMPLICIT_CONSTRUCTORS(ListenerForUpdatingBigramProbability);

        DynamicBigramListPolicy *const mBigramPolicy;
    };

    class ListenerForPlacingAndWritingValidPtNodesToBuffer
            : public DynamicPatriciaTrieReadingHelper::TraversingEventListener {
     public:
        ListenerForPlacingAndWritingValidPtNodesToBuffer(
                DynamicPatriciaTrieWritingHelper *const writingHelper,
                BufferWithExtendableBuffer *const bufferToWrite,
                hash_map_compat<int, int> *const positionMap)
                : mWritingHelper(writingHelper), mBufferToWrite(bufferToWrite),
                  mPositionMap(positionMap), mValidPtNodeCount(0),
                  mPtNodeArraySizeFieldPos(NOT_A_DICT_POS) {};

        bool onAscend() { return true; }

        bool onDescend(const int ptNodeArrayPos);

        bool onReadingPtNodeArrayTail();

        bool onVisitingPtNode(const DynamicPatriciaTrieNodeReader *const node,
                const int *const nodeCodePoints);

        hash_map_compat<int, int> *getPositionMap() const {
            return mPositionMap;
        }

     private:
        DISALLOW_IMPLICIT_CONSTRUCTORS(ListenerForPlacingAndWritingValidPtNodesToBuffer);

        DynamicPatriciaTrieWritingHelper *const mWritingHelper;
        BufferWithExtendableBuffer *const mBufferToWrite;
        hash_map_compat<int, int> *const mPositionMap;
        int mValidPtNodeCount;
        int mPtNodeArraySizeFieldPos;
    };

 private:
    DISALLOW_IMPLICIT_CONSTRUCTORS(DynamicPatriciaTrieGcEventListeners);
};
} // namespace latinime
#endif /* LATINIME_DYNAMIC_PATRICIA_TRIE_GC_EVENT_LISTENERS_H */
