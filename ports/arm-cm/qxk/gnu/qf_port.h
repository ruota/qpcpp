/// @file
/// @brief QF/C++ port to ARM Cortex-M, dual-mode QXK kernel, GNU-ARM toolset
/// @cond
///***************************************************************************
/// Last Updated for Version: 6.1.1
/// Date of the Last Update:  2018-03-05
///
///                    Q u a n t u m     L e a P s
///                    ---------------------------
///                    innovating embedded systems
///
/// Copyright (C) 2005-2018 Quantum Leaps, LLC. All rights reserved.
///
/// This program is open source software: you can redistribute it and/or
/// modify it under the terms of the GNU General Public License as published
/// by the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
///
/// Alternatively, this program may be distributed and modified under the
/// terms of Quantum Leaps commercial licenses, which expressly supersede
/// the GNU General Public License and are specifically designed for
/// licensees interested in retaining the proprietary status of their code.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
/// GNU General Public License for more details.
///
/// You should have received a copy of the GNU General Public License
/// along with this program. If not, see <http://www.gnu.org/licenses/>.
///
/// Contact information:
/// https://www.state-machine.com
/// mailto:info@state-machine.com
///***************************************************************************
/// @endcond

#ifndef qf_port_h
#define qf_port_h

// The maximum number of active objects in the application, see NOTE1
#define QF_MAX_ACTIVE           32

// The maximum number of system clock tick rates
#define QF_MAX_TICK_RATE        2

// QF interrupt disable/enable and log2()...
#if (__ARM_ARCH == 6) // Cortex-M0/M0+/M1(v6-M, v6S-M)?

    // Cortex-M0/M0+/M1(v6-M, v6S-M) interrupt disabling policy, see NOTE2
    #define QF_INT_DISABLE()    __asm volatile ("cpsid i")
    #define QF_INT_ENABLE()     __asm volatile ("cpsie i")

    // QF critical section entry/exit (unconditional interrupt disabling)
    //#define QF_CRIT_STAT_TYPE not defined
    #define QF_CRIT_ENTRY(dummy) QF_INT_DISABLE()
    #define QF_CRIT_EXIT(dummy)  QF_INT_ENABLE()

    // CMSIS threshold for "QF-aware" interrupts, see NOTE2 and NOTE4
    #define QF_AWARE_ISR_CMSIS_PRI 0

    // hand-optimized LOG2 in assembly for Cortex-M0/M0+/M1(v6-M, v6S-M)
    #define QF_LOG2(n_) QF_qlog2((n_))

#else // Cortex-M3/M4/M7

    // Cortex-M3/M4/M7 alternative interrupt disabling with PRIMASK
    #define QF_PRIMASK_DISABLE() __asm volatile ("cpsid i")
    #define QF_PRIMASK_ENABLE()  __asm volatile ("cpsie i")

    // Cortex-M3/M4/M7 interrupt disabling policy, see NOTE3 and NOTE4
    #define QF_INT_DISABLE() __asm volatile (\
        "cpsid i\n" "msr BASEPRI,%0\n" "cpsie i" :: "r" (QF_BASEPRI) : )
    #define QF_INT_ENABLE()  __asm volatile (\
        "msr BASEPRI,%0" :: "r" (0) : )

    // QF critical section entry/exit (unconditional interrupt disabling)
    //#define QF_CRIT_STAT_TYPE not defined
    #define QF_CRIT_ENTRY(dummy) QF_INT_DISABLE()
    #define QF_CRIT_EXIT(dummy)  QF_INT_ENABLE()

    // BASEPRI threshold for "QF-aware" interrupts, see NOTE3
    #define QF_BASEPRI           0x3F

    // CMSIS threshold for "QF-aware" interrupts, see NOTE5
    #define QF_AWARE_ISR_CMSIS_PRI (QF_BASEPRI >> (8 - __NVIC_PRIO_BITS))

    // Cortex-M3/M4/M7 provide the CLZ instruction for fast LOG2
    #define QF_LOG2(n_) (static_cast<uint_fast8_t>(32U - __builtin_clz(n_)))

#endif

#define QF_CRIT_EXIT_NOP()      __asm volatile ("isb")

#include "qep_port.h" // QEP port

#if (__ARM_ARCH == 6) // Cortex-M0/M0+/M1(v6-M, v6S-M)?
    // hand-optimized quick LOG2 in assembly
    extern "C" uint_fast8_t QF_qlog2(uint32_t x);
#endif // Cortex-M0/M0+/M1(v6-M, v6S-M)

#include "qxk_port.h" // QXK dual-mode kernel port
#include "qf.h"       // QF platform-independent public interface
#include "qxthread.h" // QXK extended thread

//****************************************************************************
// NOTE1:
// The maximum number of active objects QF_MAX_ACTIVE can be increased
// up to 64, if necessary. Here it is set to a lower level to save some RAM.
//
// NOTE2:
// On Cortex-M0/M0+/M1 (architecture v6-M, v6S-M), the interrupt disabling
// policy uses the PRIMASK register to disable interrupts globally. The
// QF_AWARE_ISR_CMSIS_PRI level is zero, meaning that all interrupts are
// "QF-aware".
//
// NOTE3:
// On Cortex-M3/M4/M7, the interrupt disable/enable policy uses the BASEPRI
// register (which is not implemented in Cortex-M0/M0+/M1) to disable
// interrupts only with priority lower than the threshold specified by the
// QF_BASEPRI macro. The interrupts with priorities above QF_BASEPRI (i.e.,
// with numerical priority values lower than QF_BASEPRI) are NOT disabled in
// this method. These free-running interrupts have very low ("zero") latency,
// but they are not allowed to call any QF services, because QF is unaware
// of them ("QF-unaware" interrutps). Consequently, only interrupts with
// numerical values of priorities eqal to or higher than QF_BASEPRI
// ("QF-aware" interrupts ), can call QF services.
//
// NOTE4:
// The QF_AWARE_ISR_CMSIS_PRI macro is useful as an offset for enumerating
// the "QF-aware" interrupt priorities in the applications, whereas the
// numerical values of the "QF-aware" interrupts must be greater or equal to
// QF_AWARE_ISR_CMSIS_PRI. The values based on QF_AWARE_ISR_CMSIS_PRI can be
// passed directly to the CMSIS function NVIC_SetPriority(), which shifts
// them by (8 - __NVIC_PRIO_BITS) into the correct bit position, while
// __NVIC_PRIO_BITS is the CMSIS macro defining the number of implemented
// priority bits in the NVIC. Please note that the macro QF_AWARE_ISR_CMSIS_PRI
// is intended only for applications and is not used inside the QF port, which
// remains generic and not dependent on the number of implemented priority bits
// implemented in the NVIC.
//
// NOTE5:
// The selective disabling of "QF-aware" interrupts with the BASEPRI register
// has a problem on ARM Cortex-M7 core r0p1 (see ARM-EPM-064408, errata
// 837070). The workaround recommended by ARM is to surround MSR BASEPRI with
// the CPSID i/CPSIE i pair, which is implemented in the QF_INT_DISABLE()
// macro. This workaround works also for Cortex-M3/M4 cores.
//

#endif // qf_port_h

