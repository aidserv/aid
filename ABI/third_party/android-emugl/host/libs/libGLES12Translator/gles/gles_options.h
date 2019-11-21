// Copyright 2015 Android

#ifndef GLES_GLES_OPTIONS_H_
#define GLES_GLES_OPTIONS_H_

// A macro to disallow the copy constructor and operator= functions
// This should be used in the private: declarations for a class
#define COMMON_DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&);                      \
  void operator=(const TypeName&)

namespace emugl {

    struct GlesOptions {
        static bool GLFixedAttribsEnabled() { return gl_fixed_attribs; }
        static bool GLErrorChecksEnabled() { return gl_error_checks; }

    private:
        GlesOptions();
        ~GlesOptions();
        COMMON_DISALLOW_COPY_AND_ASSIGN(GlesOptions);

#ifdef GL_FIXED_ATTRIBS
        static const bool gl_fixed_attribs = true;
#else // !GL_FIXED_ATTRIBS
        static const bool gl_fixed_attribs = false;
#endif // GL_FIXED_ATTRIBS

#ifdef GL_ERROR_CHECKS
        static const bool gl_error_checks = true;
#else // ! GL_ERROR_CHECKS
        static const bool gl_error_checks = false;
#endif // GL_ERROR_CHECKS
    };

}  // namespace emugl

#endif  // GLES_GLES_OPTIONS_H_
