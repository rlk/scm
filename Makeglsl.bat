setlocal
set PATH=%PATH%;C:\bin\

xxd.exe -i scm-label-circle.frag > scm-label-circle-frag.h
xxd.exe -i scm-label-circle.vert > scm-label-circle-vert.h
xxd.exe -i scm-label-sprite.frag > scm-label-sprite-frag.h
xxd.exe -i scm-label-sprite.vert > scm-label-sprite-vert.h

xxd.exe -i scm-render-blur.frag > scm-render-blur-frag.h
xxd.exe -i scm-render-blur.vert > scm-render-blur-vert.h
xxd.exe -i scm-render-both.frag > scm-render-both-frag.h
xxd.exe -i scm-render-both.vert > scm-render-both-vert.h
xxd.exe -i scm-render-atmo.frag > scm-render-atmo-frag.h
xxd.exe -i scm-render-atmo.vert > scm-render-atmo-vert.h
xxd.exe -i scm-render-fade.frag > scm-render-fade-frag.h
xxd.exe -i scm-render-fade.vert > scm-render-fade-vert.h

endlocal
