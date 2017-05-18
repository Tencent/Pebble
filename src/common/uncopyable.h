// Copyright (c) 2017, Tencent Inc.
// All rights reserved.
//
// Author: Richard Gu <richardgu@tencent.com>
// Created: 05/15/17
// Description:

#ifndef SRC_COMMON_UNCOPYABLE_H
#define SRC_COMMON_UNCOPYABLE_H
#pragma once

namespace common {

namespace UncopyableDetails {

class Uncopyable {
protected:
    Uncopyable() {}
    ~Uncopyable() {}

private:
    Uncopyable(const Uncopyable&);
    const Uncopyable& operator=(const Uncopyable&);
}; // class Uncopyable

} // namespace UncopyableDetails

typedef UncopyableDetails::Uncopyable Uncopyable;

#define DECLARE_UNCOPYABLE(Class) \
private: \
    Class(const Class&); \
    const Class& operator=(const Class&);

} // namespace common

#endif // SRC_COMMON_UNCOPYABLE_H
