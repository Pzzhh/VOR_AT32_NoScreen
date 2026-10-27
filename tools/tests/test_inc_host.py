"""Build the real INC controller with host I/O stubs; no board is required.

Usage: python3 test_inc_host.py /path/to/implement/Slave_Inc_Ctrl.c
Requires a C compiler (CC or cc). Generated sources/executable live in a temp dir.
"""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

HARNESS = r'''
#include <assert.h>
#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include "INC_HEADER"
static uint32_t now, updated;
static float actual;
static int valid, overcurrent, io, speed;
static uint8_t (*callback)(void);
static uint32_t ControlGetTick(void) { return now; }
static int overCurrent(void) { return overcurrent; }
static void INC_IO_Set(int value) { io = value; }
static void INC_IO_INIT(void) { io = 0; }
static void Motor_Ctl_Set(int a, int b) { assert(a == b); speed = a; }
static uint8_t Slave1_Get_Machine_state(void) { return callback != 0; }
static void Slave1_Set_Machine_Cb(uint8_t (*cb)(void)) { callback = cb; }
static uint8_t JY60_Get_Inc_Fresh(float *angle, uint32_t age) {
    if (!valid || (uint32_t)(now - updated) > age) return 0;
    if (angle) *angle = actual;
    return 1;
}
CONTROLLER_SOURCE
static void reset(float angle) {
    if (callback) { INC_Machine_Stop(); if (callback()) callback = 0; }
    actual = angle; updated = now; valid = 1; overcurrent = 0; io = speed = 0;
}
static void step(uint32_t ms, float angle, int fresh) {
    now += ms; actual = angle; if (fresh) updated = now;
    if (callback && callback()) callback = 0;
}
int main(void) {
    reset(0); assert(INC_Machine_Init(0.19f, 0, 0)); assert(!callback && !io);
    reset(0); assert(INC_Machine_Init(0.2f, 0, 0)); assert(!callback && !io);
    reset(20); assert(INC_Machine_Init(20.2f, 0, 0)); assert(!callback && !io);
    reset(0); assert(INC_Machine_Init(0.21f, 0, 0)); assert(callback);
    step(1, 0, 1); assert(io == 1 && speed > 0);
    reset(20); assert(INC_Machine_Init(15, 0, 0)); step(1, 20, 1); assert(io == 2 && speed < 0);
    reset(0); assert(INC_Machine_Init(10, 0, 0)); step(1, 10, 1);
    step(299, 10, 1); assert(INC_Machine_Get_Result() == INC_RESULT_MOVING);
    step(1, 10, 1); assert(INC_Machine_Get_Result() == INC_RESULT_OK && !callback && !io && !speed);
    reset(10); assert(INC_Machine_Init(0, 0, 0)); step(1, 0, 1); step(300, 0, 1);
    assert(INC_Machine_Get_Result() == INC_RESULT_OK && !callback && !io && !speed);
    reset(0); assert(INC_Machine_Init(10, 0, 0)); step(1, 10, 1); step(200, 9, 1);
    step(1, 10, 1); step(299, 10, 1); assert(callback); step(1, 10, 1); assert(!callback);
    reset(0); assert(INC_Machine_Init(10, 0, 0)); step(1001, 0, 0);
    assert(INC_Machine_Get_Result() == INC_RESULT_SENSOR && !callback && !io && !speed);
    reset(0); assert(INC_Machine_Init(10, 0, 0)); overcurrent = 1; step(1, 0, 1);
    assert(INC_Machine_Get_Result() == INC_RESULT_OVERCURRENT && !callback && !io && !speed);
    reset(0); assert(INC_Machine_Init(10, 0, 0)); step(60000, 0, 1);
    assert(INC_Machine_Get_Result() == INC_RESULT_TIMEOUT && !callback && !io && !speed);
    /* Stationary at a quantization edge: old int(error*10) alternated 2/3. */
    reset(10.301f); assert(INC_Machine_Init(10, 0, 0));
    { int n; for (n = 0; n < 99; ++n) step(100, n % 2 ? 10.301f : 10.299f, 1); }
    assert(callback && INC_Machine_Get_Result() == INC_RESULT_MOVING);
    step(100, 10.301f, 1);
    assert(INC_Machine_Get_Result() == INC_RESULT_STALLED && !callback && !io && !speed);
    /* Repeated entries/exits at the completion tolerance must also be bounded. */
    reset(10.201f); assert(INC_Machine_Init(10, 0, 0));
    { int n; for (n = 0; n < 99; ++n) step(100, n % 2 ? 10.201f : 10.199f, 1); }
    step(100, 10.201f, 1);
    assert(INC_Machine_Get_Result() == INC_RESULT_STALLED && !callback && !io && !speed);
    /* Small genuine progress accumulates, even across old integer steps. */
    reset(11); assert(INC_Machine_Init(10, 0, 0));
    step(5000, 10.96f, 1); step(4000, 10.89f, 1);
    step(5000, 10.85f, 1); assert(INC_Machine_Get_Result() == INC_RESULT_MOVING);
    step(4000, 10.78f, 1); assert(INC_Machine_Get_Result() == INC_RESULT_MOVING);
    step(10000, 10.78f, 1); assert(INC_Machine_Get_Result() == INC_RESULT_STALLED && !io && !speed);
    /* Wobble larger than the previous 0.02 threshold is still not progress. */
    reset(10.35f); assert(INC_Machine_Init(10, 0, 0));
    step(9000, 10.31f, 1); assert(INC_Machine_Get_Result() == INC_RESULT_MOVING);
    step(1000, 10.35f, 1); assert(INC_Machine_Get_Result() == INC_RESULT_STALLED && !io && !speed);
    /* Moving in the wrong direction is never allowed to reset progress. */
    reset(11); assert(INC_Machine_Init(10, 0, 0)); step(9999, 12, 1);
    step(1, 13, 1); assert(INC_Machine_Get_Result() == INC_RESULT_STALLED && !io && !speed);
    reset(11); now = UINT32_MAX - 5000; updated = now;
    assert(INC_Machine_Init(10, 0, 0)); step(10000, 11, 1);
    assert(INC_Machine_Get_Result() == INC_RESULT_STALLED && !io && !speed);
    reset(0); valid = 0; assert(!INC_Machine_Init(10, 0, 0)); assert(!callback && !io);
    reset(0); assert(INC_Machine_Manual_Ctrl(1, 1)); step(1, 0, 1); assert(io == 2 && speed < 0);
    step(1000, 0, 1); assert(!callback && !io && !speed);
    reset(0); assert(!INC_Machine_Init(NAN, 0, 0)); assert(!INC_Machine_Init(51, 0, 0));
    reset(0); assert(INC_Machine_Init(10, 0, 0)); step(1, 0, 1);
    assert(INC_Machine_Stop()); assert(!io && !speed); step(1, 0, 1); assert(!callback);
    reset(0); now = UINT32_MAX - 100; updated = now; assert(INC_Machine_Init(10, 0, 0));
    step(1, 10, 1); step(300, 10, 1); assert(INC_Machine_Get_Result() == INC_RESULT_OK);
    puts("INC host tests passed: tolerance, direction, stability, zero, sensor, overcurrent, timeout, cancellation, clock wrap, quantization jitter, no-progress protection");
    return 0;
}
'''


def main():
    source = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).resolve().parents[2] / "implement" / "Slave_Inc_Ctrl.c"
    text = source.read_text(encoding="utf-8")
    controller = "\n".join(line for line in text.splitlines() if not line.startswith("#include"))
    code = HARNESS.replace("INC_HEADER", source.with_suffix(".h").resolve().as_posix()).replace("CONTROLLER_SOURCE", controller)
    with tempfile.TemporaryDirectory(prefix="vor-inc-host-") as folder:
        generated = Path(folder) / "test.c"
        executable = Path(folder) / "test"
        generated.write_text(code, encoding="utf-8")
        subprocess.run([os.environ.get("CC", "cc"), "-std=c99", "-Wall", "-Wextra", "-Werror", str(generated), "-lm", "-o", str(executable)], check=True)
        subprocess.run([str(executable)], check=True)


if __name__ == "__main__": main()
