#pragma once

// Android system might want to allocate some color buffers with formats
// that are not compatible with most OpenGL implementations,
// such as YV12.
// In this situation, we need to convert to some OpenGL format such as
// RGB888 that actually works.
// While we can do some of this conversion in the guest gralloc driver itself
// (which would make ColorBuffer see only the OpenGL formatted pixels),
// it may be advantageous to do the conversion on the host as well.
// FrameworkFormat tracks whether the incoming color buffer from the guest
// can be directly used as a GL texture (FRAMEWORK_FORMAT_GL_COMPATIBLE).
// Otherwise, we need to know which format it is (e.g., FRAMEWORK_FORMAT_YV12)
// and convert on the host.
enum FrameworkFormat {
    FRAMEWORK_FORMAT_GL_COMPATIBLE = 0,
    FRAMEWORK_FORMAT_YV12 = 1,
    FRAMEWORK_FORMAT_YUV_420_888 = 2,
};
