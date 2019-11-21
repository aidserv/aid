=====gles1=====gles1>>>>>Emulator GLESv1->2 Translator>>>>>gles2=====gles2=====

Not every rendering backend that the emulator supports will support GLESv1.
For example, ANGLE only supports GLESv2.

We need a GLESv1->GLESv2 translation layer in order to take care of GLESv1 use
cases.

This library, libGLES12Translator, is a general solution where if our
currently-utilized GLES backend (such as ANGLE) does not provide libGLES_CM.dll
(the GLESv1 dll), libGLES12Translator will be used instead, and hook into the
underlying libEGL.dll and libGLESv2.dll.

=====gles1=====gles1>>>>>Where this came from originally>>>gles2=====gles2=====

The GLESv1->2 translator in this library is derived from the
mods/graphics_translation in ARC, Android Runtime for Chrome.

ARC is for running Android apps in Chrome and has a GLESv1->2 translator that
is used to translate Android GLESv1 apps to run on the underlying WebGL (or
ANGLE) which is only GLESv2.

More information + the original source can be found from the Chromium git repo
at

https://chromium.googlesource.com/arc/arc/

under the directory mods/graphics_translation.

A bit of history for code archaeology purposes:

This is all based on a previous effort to port GLESv1->2 translator to the
emulator.  That effort itself was based off of the master branch of the ARC
repo around that time.

The closest version of the arc repository has been the master branch as of
around the end of September 2015. Not many changes were happening in ARC around
that time; we can consider this October 6, 2015 version of arc:

https://chromium.googlesource.com/arc/arc/+/016806e172e7149ba42e9b69e0ec876b1e898a4e

as the canonical ARC version off of which we base this GLESv1->2 translator.
In fact, graphics_translation itself had no changes whatsover since Sept 10,
2015 until at least June 23, 2016.

=====gles1=====gles1>>>>>An EGL-less solution>>>>>>>>>>>>>>gles2=====gles2=====

There is one heavy modification to the translator that these CLs do, and it is
to remove the EGL machinery from graphics_translation.  The egl/ subdirectory
of the original graphics_translation has been completely removed.

The idea is that in libOpenglRender on the host, we already have sufficient
information to tell graphics_translation everything without needing
graphics_translattion to have an EGL component.

gles12tr.entries in libOpenGLESDispatch contains the extra entry points that
we need in order to be able to keep the GLESv1 emulator on track.

=====gles1=====gles1>>>>>Linking underlying GLESv2>>>>>>>>>gles2=====gles2=====

In order to emulate GLESv1 on top of GLESv2, this GLESv1->2 translation library
expects the presence of actual implementation GLESv2 API. When we are using
GLESv1 emulation, we need to link against GLESv2 functions. The entry point
create_underlying_api returns a pointer to a struct that will hold the
necessary GLESv2 functions, which we then link at runtime. The GLESv1 emulation
library then notices what we linked when we pass a pointer to this struct to
GLESv1 context creation calls.

=====gles1=====gles1>>>>>Current state of the library>>>>>>gles2=====gles2=====

The library can now be used. When a custom OpenGL backend (e.g., -gpu angle or
-gpu swiftshader) is used, if that backend does not provide the libGLES_CM.dll
in addition to the usual libEGL.dll and libGLESv2.dll, we will automatically
fall back to GLESv1 emulation using this GLESv1->2 translation library.

=====gles1=====gles1>>>>>Known Issues / WIP Opportunities>>gles2=====gles2=====

Right now, this translation library seems to need to use ARC's RefBase and
StrongPointer in order to not crash during use (from smoke tests done by
experimentally hooking up the library to the emulator).

There are various rendering errors:

    - Texture-billboards don't seem to work correctly, as in the boot animation
      and the GLESv1 API demos such as Sprite Text. The textures simply don't
      show up.

    - Only GL_SMOOTH is available as a shading parameter, which makes the Kube
      API demo look funny.
