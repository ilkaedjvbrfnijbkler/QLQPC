#pragma once

#include "timing/qpc_sync_math.hpp"

namespace qlqpc::timing::detail
{
    extern qpc_fps_estimator fps_estimator;
    extern double cur_sub_ms_frac;
    extern double cur_exact_render_ms;

    void reset_prediction_session();
}
