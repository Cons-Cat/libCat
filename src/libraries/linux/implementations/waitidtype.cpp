// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <linux>

enum WaitIdType
{
    P_ALL = 0,
    P_PID = 1,
    P_PGID = 2,
    P_PIDFD = 3
};
