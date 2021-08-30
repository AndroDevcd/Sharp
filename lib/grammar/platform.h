//
// Created by BNunnally on 8/26/2021.
//

#ifndef SHARP_PLATFORM_H
#define SHARP_PLATFORM_H

enum Platform {

    /**
     * Jan 18, 2018 Build release of Sharp 0.2.0, this contains all the base level
     * support for sharp as well as major performance enhancments and improvements
     * throughout the platform.
     */
    alpha = 2,

    /**
     * Aug 10, 2021 Build release of Sharp 0.3.0
     *
     * This version offers:
     * - Greater stability in the task scheduling system
     * - Newly revised compiler that includes:
     *      - Faster compile times
     *      - Better code generation system
     *      - Significantly improved code optimization system
     *      - Supports compiled projects for the language
     */
    alpha_cr2 = 3
};

#endif //SHARP_PLATFORM_H
