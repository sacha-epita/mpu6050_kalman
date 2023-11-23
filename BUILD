
load("//BazelUtilities:default_hosts.bzl", "host_windows_x86_64")
load("//BazelUtilities/internal_toolchains/arm-none-eabi/platforms/stm32:STM32F4.bzl", "stm32f4_gen_toolchain", "STM32F4_FAMILLY")
load("//BazelUtilities/internal_toolchains/arm-none-eabi/platforms/stm32:STM32.bzl", "stm32_binary")

stm32_binary(
    name = "mpu6050_kalman",
    hosts = host_windows_x86_64,
    mcu_id = "STM32F401CCU6",
    srcs = [
       "//Core:Srcs",
    ],
    deps = [
        "//Core:Lib",
        "//Drivers:Drivers",
    ],
)

stm32f4_gen_toolchain(
    host_windows_x86_64,
    "STM32F401CCU6",
    mcu_ldscript = "STM32F401CCUx_FLASH.ld",
    mcu_device_group = "STM32F401xC",
    mcu_startupfile = "startup_stm32f401xc.s",
    use_mcu_constraint = True,
    gen_platform = True,
    copts = [
        "-IEigen",
    ],
    cxxopts = [
        "-std=c++20",
        "-Wno-int-in-bool-context",
        "-Wno-deprecated-declarations",
        "-Wno-deprecated-enum-enum-conversion",
        "-Wno-volatile"
    ],
    linkopts = [
        "-lstdc++",
        "-u _printf_float",
    ]
)
