# WORKSPACE

workspace(name = "mpu6050_kalman")

load("//BazelUtilities/internal_toolchains/arm-none-eabi:arm-none-eabi.bzl", "arm_none_eabi_deps")
load("//BazelUtilities:default_hosts.bzl", "host_windows_x86_64")
load("//BazelUtilities/internal_toolchains/arm-none-eabi/platforms/stm32:STM32F4.bzl", "stm32f4_regsiter_toolchain")

arm_none_eabi_deps()
stm32f4_regsiter_toolchain(host_windows_x86_64, "STM32F401CCU6")
