# BUILD

filegroup(
    name = "Include",
    srcs =  glob(["STM32F4xx_HAL_Driver/Inc/*.h"]) +
            glob(["STM32F4xx_HAL_Driver/Inc/Legacy/*.h"]) +
            glob(["CMSIS/Device/ST/STM32F4xx/Include/*.h"]) +
            glob(["CMSIS/Include/*.h"]),
    visibility = ["//visibility:public"],
)

cc_library(
    name = "Drivers",
    srcs = glob(["STM32F4xx_HAL_Driver/Src/*.c"]),
    hdrs = [ ":Include", "//Core:Include" ],
    visibility = ["//visibility:public"],
)

# linkstatic = 1,
# visibility = ["//visibility:public"],
# visibility = ["//Core:__pkg__"],
