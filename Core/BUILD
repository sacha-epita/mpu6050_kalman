load("@rules_cc//cc:defs.bzl", "cc_library")

filegroup(
    name = "Include",
    srcs =  glob(["Inc/*.h"]),
    visibility = ["//visibility:public"]
)

filegroup(
    name = "Srcs",
    srcs =  glob([ "Src/*.c" ]) + glob([ "Src/*.cpp" ]),
    visibility = ["//visibility:public"]
)

cc_library(
    name = "Lib",
    hdrs = glob([ "Inc/*.h" ]) + [ ":Include", "//Drivers:Include", "//Eigen:Eigen" ],
    visibility = ["//visibility:public"],
)


