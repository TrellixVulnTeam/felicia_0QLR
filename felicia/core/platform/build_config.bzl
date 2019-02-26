# Copyright 2015 The TensorFlow Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# ==============================================================================
# Modifications copyright (C) 2019 felicia

# Platform-specific build configurations.

load("@com_google_protobuf//:protobuf.bzl", "proto_gen")
load(
    "//felicia:felicia.bzl",
    "if_darwin",
    "if_not_windows",
)

# Appends a suffix to a list of deps.
def fel_deps(deps, suffix):
    fel_deps = []

    # If the package name is in shorthand form (ie: does not contain a ':'),
    # expand it to the full name.
    for dep in deps:
        fel_dep = dep

        if not ":" in dep:
            dep_pieces = dep.split("/")
            fel_dep += ":" + dep_pieces[len(dep_pieces) - 1]

        fel_deps += [fel_dep + suffix]

    return fel_deps

def _proto_cc_hdrs(srcs, use_grpc_plugin = False):
    ret = [s[:-len(".proto")] + ".pb.h" for s in srcs]
    if use_grpc_plugin:
        ret += [s[:-len(".proto")] + ".grpc.pb.h" for s in srcs]
    return ret

def _proto_cc_srcs(srcs, use_grpc_plugin = False):
    ret = [s[:-len(".proto")] + ".pb.cc" for s in srcs]
    if use_grpc_plugin:
        ret += [s[:-len(".proto")] + ".grpc.pb.cc" for s in srcs]
    return ret

def _proto_py_outs(srcs, use_grpc_plugin = False):
    ret = [s[:-len(".proto")] + "_pb2.py" for s in srcs]
    if use_grpc_plugin:
        ret += [s[:-len(".proto")] + "_pb2_grpc.py" for s in srcs]
    return ret

# Re-defined protocol buffer rule to allow building "header only" protocol
# buffers, to avoid duplicate registrations. Also allows non-iterable cc_libs
# containing select() statements.
def cc_proto_library(
        name,
        srcs = [],
        deps = [],
        cc_libs = [],
        include = None,
        protoc = "@com_google_protobuf//:protoc",
        internal_bootstrap_hack = False,
        use_grpc_plugin = False,
        use_grpc_namespace = False,
        default_header = False,
        **kargs):
    """Bazel rule to create a C++ protobuf library from proto source files.

    Args:
      name: the name of the cc_proto_library.
      srcs: the .proto files of the cc_proto_library.
      deps: a list of dependency labels; must be cc_proto_library.
      cc_libs: a list of other cc_library targets depended by the generated
          cc_library.
      include: a string indicating the include path of the .proto files.
      protoc: the label of the protocol compiler to generate the sources.
      internal_bootstrap_hack: a flag indicate the cc_proto_library is used only
          for bootstraping. When it is set to True, no files will be generated.
          The rule will simply be a provider for .proto files, so that other
          cc_proto_library can depend on it.
      use_grpc_plugin: a flag to indicate whether to call the grpc C++ plugin
          when processing the proto files.
      default_header: Controls the naming of generated rules. If True, the `name`
          rule will be header-only, and an _impl rule will contain the
          implementation. Otherwise the header-only rule (name + "_headers_only")
          must be referred to explicitly.
      **kargs: other keyword arguments that are passed to cc_library.
    """

    includes = []
    if include != None:
        includes = [include]

    if internal_bootstrap_hack:
        # For pre-checked-in generated files, we add the internal_bootstrap_hack
        # which will skip the codegen action.
        proto_gen(
            name = name + "_genproto",
            srcs = srcs,
            includes = includes,
            protoc = protoc,
            visibility = ["//visibility:public"],
            deps = [s + "_genproto" for s in deps],
        )

        # An empty cc_library to make rule dependency consistent.
        native.cc_library(
            name = name,
            **kargs
        )
        return

    grpc_cpp_plugin = None
    plugin_options = []
    if use_grpc_plugin:
        grpc_cpp_plugin = "@com_github_grpc_grpc//:grpc_cpp_plugin"
        if use_grpc_namespace:
            plugin_options = ["services_namespace=grpc"]

    gen_srcs = _proto_cc_srcs(srcs, use_grpc_plugin)
    gen_hdrs = _proto_cc_hdrs(srcs, use_grpc_plugin)
    outs = gen_srcs + gen_hdrs

    proto_gen(
        name = name + "_genproto",
        srcs = srcs,
        outs = outs,
        gen_cc = 1,
        includes = includes,
        plugin = grpc_cpp_plugin,
        plugin_language = "grpc",
        plugin_options = plugin_options,
        protoc = protoc,
        visibility = ["//visibility:public"],
        deps = [s + "_genproto" for s in deps],
    )

    grpc_dpes = []
    if use_grpc_plugin:
        grpc_dpes.append("//felicia:grpc++")

    if default_header:
        header_only_name = name
        impl_name = name + "_impl"
    else:
        header_only_name = name + "_headers_only"
        impl_name = name

    native.cc_library(
        name = impl_name,
        srcs = gen_srcs,
        hdrs = gen_hdrs,
        deps = cc_libs + deps + grpc_dpes,
        includes = includes,
        **kargs
    )
    native.cc_library(
        name = header_only_name,
        deps = ["@com_google_protobuf//:protobuf_headers"] + if_static([impl_name]),
        hdrs = gen_hdrs,
        **kargs
    )

# Re-defined protocol buffer rule to bring in the change introduced in commit
# https://github.com/google/protobuf/commit/294b5758c373cbab4b72f35f4cb62dc1d8332b68
# which was not part of a stable protobuf release in 04/2018.
# TODO(jsimsa): Remove this once the protobuf dependency version is updated
# to include the above commit.
def py_proto_library(
        name,
        srcs = [],
        deps = [],
        py_libs = [],
        py_extra_srcs = [],
        include = None,
        default_runtime = "@com_google_protobuf//:protobuf_python",
        protoc = "@com_google_protobuf//:protoc",
        use_grpc_plugin = False,
        **kargs):
    """Bazel rule to create a Python protobuf library from proto source files

    NOTE: the rule is only an internal workaround to generate protos. The
    interface may change and the rule may be removed when bazel has introduced
    the native rule.

    Args:
      name: the name of the py_proto_library.
      srcs: the .proto files of the py_proto_library.
      deps: a list of dependency labels; must be py_proto_library.
      py_libs: a list of other py_library targets depended by the generated
          py_library.
      py_extra_srcs: extra source files that will be added to the output
          py_library. This attribute is used for internal bootstrapping.
      include: a string indicating the include path of the .proto files.
      default_runtime: the implicitly default runtime which will be depended on by
          the generated py_library target.
      protoc: the label of the protocol compiler to generate the sources.
      use_grpc_plugin: a flag to indicate whether to call the Python C++ plugin
          when processing the proto files.
      **kargs: other keyword arguments that are passed to cc_library.
    """
    outs = _proto_py_outs(srcs, use_grpc_plugin)

    includes = []
    if include != None:
        includes = [include]

    grpc_python_plugin = None
    if use_grpc_plugin:
        grpc_python_plugin = "//external:grpc_python_plugin"
        # Note: Generated grpc code depends on Python grpc module. This dependency
        # is not explicitly listed in py_libs. Instead, host system is assumed to
        # have grpc installed.

    proto_gen(
        name = name + "_genproto",
        srcs = srcs,
        outs = outs,
        gen_py = 1,
        includes = includes,
        plugin = grpc_python_plugin,
        plugin_language = "grpc",
        protoc = protoc,
        visibility = ["//visibility:public"],
        deps = [s + "_genproto" for s in deps],
    )

    if default_runtime and not default_runtime in py_libs + deps:
        py_libs = py_libs + [default_runtime]

    native.py_library(
        name = name,
        srcs = outs + py_extra_srcs,
        deps = py_libs + deps,
        imports = includes,
        **kargs
    )

def fel_proto_library_cc(
        name,
        srcs = [],
        has_services = None,
        protodeps = [],
        visibility = [],
        testonly = 0,
        cc_libs = [],
        cc_stubby_versions = None,
        cc_grpc_version = None,
        j2objc_api_version = 1,
        cc_api_version = 1,
        js_codegen = "jspb",
        default_header = False):
    js_codegen = js_codegen  # unused argument
    native.filegroup(
        name = name + "_proto_srcs",
        srcs = srcs + fel_deps(protodeps, "_proto_srcs"),
        testonly = testonly,
        visibility = visibility,
    )

    use_grpc_plugin = None
    use_grpc_namespace = None
    if cc_grpc_version:
        use_grpc_plugin = True
        use_grpc_namespace = True

    cc_deps = fel_deps(protodeps, "_cc")
    cc_name = name + "_cc"
    if not srcs:
        # This is a collection of sub-libraries. Build header-only and impl
        # libraries containing all the sources.
        proto_gen(
            name = cc_name + "_genproto",
            protoc = "@com_google_protobuf//:protoc",
            visibility = ["//visibility:public"],
            deps = [s + "_genproto" for s in cc_deps],
        )
        native.cc_library(
            name = cc_name,
            deps = cc_deps + ["@com_google_protobuf//:protobuf_headers"] + if_static([name + "_cc_impl"]),
            testonly = testonly,
            visibility = visibility,
        )
        native.cc_library(
            name = cc_name + "_impl",
            deps = [s + "_impl" for s in cc_deps] + ["@com_google_protobuf//:cc_wkt_protos"],
        )

        return

    cc_proto_library(
        name = cc_name,
        testonly = testonly,
        srcs = srcs,
        cc_libs = cc_libs + if_static(
            ["@com_google_protobuf//:protobuf"],
            ["@com_google_protobuf//:protobuf_headers"],
        ),
        copts = if_not_windows([
            "-Wno-unknown-warning-option",
            "-Wno-unused-but-set-variable",
            "-Wno-sign-compare",
        ]),
        default_header = default_header,
        protoc = "@com_google_protobuf//:protoc",
        use_grpc_plugin = use_grpc_plugin,
        use_grpc_namespace = use_grpc_namespace,
        visibility = visibility,
        deps = cc_deps + ["@com_google_protobuf//:cc_wkt_protos"],
    )

def fel_proto_library_py(
        name,
        srcs = [],
        protodeps = [],
        deps = [],
        visibility = [],
        testonly = 0,
        srcs_version = "PY2AND3",
        use_grpc_plugin = False):
    py_deps = fel_deps(protodeps, "_py")
    py_name = name + "_py"
    if not srcs:
        # This is a collection of sub-libraries. Build header-only and impl
        # libraries containing all the sources.
        proto_gen(
            name = py_name + "_genproto",
            protoc = "@com_google_protobuf//:protoc",
            visibility = ["//visibility:public"],
            deps = [s + "_genproto" for s in py_deps],
        )
        native.py_library(
            name = py_name,
            deps = py_deps + ["@com_google_protobuf//:protobuf_python"],
            testonly = testonly,
            visibility = visibility,
        )
        return

    py_proto_library(
        name = py_name,
        testonly = testonly,
        srcs = srcs,
        default_runtime = "@com_google_protobuf//:protobuf_python",
        protoc = "@com_google_protobuf//:protoc",
        srcs_version = srcs_version,
        use_grpc_plugin = use_grpc_plugin,
        visibility = visibility,
        deps = deps + py_deps + ["@com_google_protobuf//:protobuf_python"],
    )

def fel_proto_library(
        name,
        srcs = [],
        has_services = None,
        protodeps = [],
        visibility = [],
        testonly = 0,
        cc_libs = [],
        cc_api_version = 1,
        cc_grpc_version = None,
        j2objc_api_version = 1,
        js_codegen = "jspb",
        provide_cc_alias = False,
        default_header = False):
    """Make a proto library, possibly depending on other proto libraries."""
    _ignore = (js_codegen, provide_cc_alias)

    fel_proto_library_cc(
        name = name,
        testonly = testonly,
        srcs = srcs,
        cc_grpc_version = cc_grpc_version,
        cc_libs = cc_libs,
        default_header = default_header,
        protodeps = protodeps,
        visibility = visibility,
    )

    fel_proto_library_py(
        name = name,
        testonly = testonly,
        srcs = srcs,
        protodeps = protodeps,
        srcs_version = "PY2AND3",
        use_grpc_plugin = has_services,
        visibility = visibility,
    )

def if_static(extra_deps, otherwise = []):
    return select({
        "//felicia:framework_shared_object": otherwise,
        "//conditions:default": extra_deps,
    })

def fel_platform_files(files, exclude = []):
    base_files = native.glob(["platform/" + f for f in files], exclude = exclude)
    windows_files = native.glob(["platform/win/" + f for f in files], exclude = exclude)
    posix_files = native.glob(["platform/posix/" + f for f in files], exclude = exclude)
    darwin_files = native.glob(["platform/mac/" + f for f in files], exclude = exclude)
    return base_files + if_darwin(darwin_files) + select({
        "//felicia:windows": windows_files,
        "//conditions:default": posix_files,
    })

def fel_additional_lib_hdrs(exclude = []):
    windows_hdrs = native.glob([
        "platform/win/*.h",
    ], exclude = exclude)
    posix_hdrs = native.glob([
        "platform/posix/*.h",
    ], exclude = exclude)
    darwin_hdrs = native.glob([
        "platform/mac/*.h",
    ], exclude = exclude)
    return if_darwin(darwin_hdrs) + select({
        "//felicia:windows": windows_hdrs,
        "//conditions:default": posix_hdrs,
    })

def fel_additional_lib_srcs(exclude = []):
    windows_srcs = native.glob([
        "platform/win/*.cc",
    ], exclude = exclude)
    posix_srcs = native.glob([
        "platform/posix/*.cc",
    ], exclude = exclude)
    darwin_srcs = native.glob([
        "platform/mac/*.cc",
    ], exclude = exclude)
    return if_darwin(darwin_srcs) + select({
        "//felicia:windows": windows_srcs,
        "//conditions:default": posix_srcs,
    })

def fel_additional_lib_deps():
    return [
        "@com_google_googletest//:gtest",
        "@com_google_protobuf//:protobuf",
        "//third_party/chromium/base:base",
        "//third_party/chromium/net:net",
    ]