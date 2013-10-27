/*
 * Copyright (C) 2008 The Android Open Source Project
 * Copyright (c) 2010-2012, The Linux Foundation. All rights reserved.
 * Not a Contribution, Apache license notifications and license are retained
 * for attribution purposes only.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#include "overlayRotator.h"
#include "overlayUtils.h"
#include "mdp_version.h"
#include "gr.h"

namespace ovutils = overlay::utils;

namespace overlay {

Rotator::~Rotator() {}

Rotator* Rotator::getRotator() {
    int type = getRotatorHwType();
    if(type == TYPE_MDP) {
        return new MdpRot(); //will do reset
    } else if(type == TYPE_MDSS) {
        return new MdssRot();
    } else {
        ALOGE("%s Unknown h/w type %d", __FUNCTION__, type);
        return NULL;
    }
}

uint32_t Rotator::calcOutputBufSize(const utils::Whf& destWhf) {
    //dummy aligned w & h.
    int alW = 0, alH = 0;
    int halFormat = ovutils::getHALFormat(destWhf.format);
    //A call into gralloc/memalloc
    return getBufferSizeAndDimensions(
            destWhf.w, destWhf.h, halFormat, alW, alH);
}

int Rotator::getRotatorHwType() {
    int mdpVersion = qdutils::MDPVersion::getInstance().getMDPVersion();
    if (mdpVersion == qdutils::MDSS_V5)
        return TYPE_MDSS;
    return TYPE_MDP;
}

bool RotMem::close() {
    bool ret = true;
    for(uint32_t i=0; i < RotMem::MAX_ROT_MEM; ++i) {
        // skip current, and if valid, close
        if(m[i].valid()) {
            if(m[i].close() == false) {
                ALOGE("%s error in closing rot mem %d", __FUNCTION__, i);
                ret = false;
            }
        }
    }
    return ret;
}

RotMgr::RotMgr() {
    for(int i = 0; i < MAX_ROT_SESS; i++) {
        mRot[i] = 0;
    }
    mUseCount = 0;
}

RotMgr::~RotMgr() {
    clear();
}

void RotMgr::configBegin() {
    //Reset the number of objects used
    mUseCount = 0;
}

void RotMgr::configDone() {
    //Remove the top most unused objects. Videos come and go.
    for(int i = mUseCount; i < MAX_ROT_SESS; i++) {
        if(mRot[i]) {
            delete mRot[i];
            mRot[i] = 0;
        }
    }
}

Rotator* RotMgr::getNext() {
    //Return a rot object, creating one if necessary
    overlay::Rotator *rot = NULL;
    if(mUseCount >= MAX_ROT_SESS) {
        ALOGE("%s, MAX rotator sessions reached", __func__);
    } else {
        if(mRot[mUseCount] == NULL)
            mRot[mUseCount] = overlay::Rotator::getRotator();
        rot = mRot[mUseCount++];
    }
    return rot;
}

void RotMgr::clear() {
    //Brute force obj destruction, helpful in suspend.
    for(int i = 0; i < MAX_ROT_SESS; i++) {
        if(mRot[i]) {
            delete mRot[i];
            mRot[i] = 0;
        }
    }
    mUseCount = 0;
}

void RotMgr::getDump(char *buf, size_t len) {
    for(int i = 0; i < MAX_ROT_SESS; i++) {
        if(mRot[i]) {
            mRot[i]->getDump(buf, len);
        }
    }
    char str[32] = {'\0'};
    snprintf(str, 32, "\n================\n");
    strncat(buf, str, strlen(str));
}

}