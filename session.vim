let SessionLoad = 1
let s:so_save = &g:so | let s:siso_save = &g:siso | setg so=0 siso=0 | setl so=-1 siso=-1
let v:this_session=expand("<sfile>:p")
silent only
silent tabonly
cd ~/Documents/master/pebbling-pdr
if expand('%') == '' && !&modified && line('$') <= 1 && getline(1) == ''
  let s:wipebuf = bufnr('%')
endif
set shortmess=aoO
argglobal
%argdel
$argadd src/model/peterson/peterson-result.cpp
set stal=2
tabnew
tabnew
tabrewind
edit src/model/peterson/peterson.cpp
let s:save_splitbelow = &splitbelow
let s:save_splitright = &splitright
set splitbelow splitright
wincmd _ | wincmd |
vsplit
wincmd _ | wincmd |
vsplit
wincmd _ | wincmd |
vsplit
3wincmd h
wincmd w
wincmd w
wincmd w
let &splitbelow = s:save_splitbelow
let &splitright = s:save_splitright
wincmd t
let s:save_winminheight = &winminheight
let s:save_winminwidth = &winminwidth
set winminheight=0
set winheight=1
set winminwidth=0
set winwidth=1
exe 'vert 1resize ' . ((&columns * 90 + 181) / 362)
exe 'vert 2resize ' . ((&columns * 90 + 181) / 362)
exe 'vert 3resize ' . ((&columns * 89 + 181) / 362)
exe 'vert 4resize ' . ((&columns * 90 + 181) / 362)
argglobal
balt inc/model/peterson/peterson.h
setlocal fdm=manual
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
silent! normal! zE
let &fdl = &fdl
let s:l = 1285 - ((73 * winheight(0) + 39) / 79)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 1285
normal! 07|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/auxiliary/z3-ext.cpp") | buffer ~/Documents/master/pebbling-pdr/src/auxiliary/z3-ext.cpp | else | edit ~/Documents/master/pebbling-pdr/src/auxiliary/z3-ext.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/auxiliary/z3-ext.cpp
endif
balt ~/Documents/master/pebbling-pdr/inc/auxiliary/z3-ext.h
setlocal fdm=manual
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
silent! normal! zE
let &fdl = &fdl
let s:l = 41 - ((40 * winheight(0) + 39) / 79)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 41
normal! 021|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/testing/z3pdr.cpp") | buffer ~/Documents/master/pebbling-pdr/src/testing/z3pdr.cpp | else | edit ~/Documents/master/pebbling-pdr/src/testing/z3pdr.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/testing/z3pdr.cpp
endif
balt ~/Documents/master/pebbling-pdr/src/model/peterson/peterson.cpp
setlocal fdm=manual
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
silent! normal! zE
let &fdl = &fdl
let s:l = 192 - ((34 * winheight(0) + 39) / 79)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 192
normal! 03|
lcd ~/Documents/master/pebbling-pdr
wincmd w
argglobal
if bufexists("~/Documents/master/pebbling-pdr/src/model/pdr/pdr-model.cpp") | buffer ~/Documents/master/pebbling-pdr/src/model/pdr/pdr-model.cpp | else | edit ~/Documents/master/pebbling-pdr/src/model/pdr/pdr-model.cpp | endif
if &buftype ==# 'terminal'
  silent file ~/Documents/master/pebbling-pdr/src/model/pdr/pdr-model.cpp
endif
balt ~/Documents/master/pebbling-pdr/inc/model/pdr/pdr-model.h
setlocal fdm=manual
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
silent! normal! zE
let &fdl = &fdl
let s:l = 169 - ((21 * winheight(0) + 39) / 79)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 169
normal! 020|
lcd ~/Documents/master/pebbling-pdr
wincmd w
exe 'vert 1resize ' . ((&columns * 90 + 181) / 362)
exe 'vert 2resize ' . ((&columns * 90 + 181) / 362)
exe 'vert 3resize ' . ((&columns * 89 + 181) / 362)
exe 'vert 4resize ' . ((&columns * 90 + 181) / 362)
tabnext
edit ~/Documents/master/pebbling-pdr/src/model/peterson/peterson-result.cpp
argglobal
1argu
balt ~/Documents/master/pebbling-pdr/src/algo/result.cpp
setlocal fdm=manual
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
silent! normal! zE
let &fdl = &fdl
let s:l = 1 - ((0 * winheight(0) + 41) / 82)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 1
normal! 0
lcd ~/Documents/master/pebbling-pdr
tabnext
edit ~/Documents/master/pebbling-pdr/src/algo/result.cpp
argglobal
balt ~/Documents/master/pebbling-pdr/src/model/pdr/pdr-model.cpp
setlocal fdm=manual
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
silent! normal! zE
let &fdl = &fdl
let s:l = 1 - ((0 * winheight(0) + 41) / 82)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 1
normal! 0
lcd ~/Documents/master/pebbling-pdr
tabnext 1
set stal=1
badd +1284 ~/Documents/master/pebbling-pdr/src/model/peterson/peterson.cpp
badd +1 ~/Documents/master/pebbling-pdr/src/testing/stats.cpp
badd +82 ~/Documents/master/pebbling-pdr/inc/model/peterson/peterson.h
badd +169 ~/Documents/master/pebbling-pdr/src/model/pdr/pdr-model.cpp
badd +357 ~/Documents/master/pebbling-pdr/src/model/expr.cpp
badd +159 ~/Documents/master/pebbling-pdr/inc/algo/pdr.h
badd +1 ~/Documents/master/pebbling-pdr/inc/testing/z3-pebbling-model.h
badd +51 ~/Documents/master/pebbling-pdr/inc/model/pdr/pdr-model.h
badd +192 ~/Documents/master/pebbling-pdr/src/testing/z3pdr.cpp
badd +33 ~/Documents/master/pebbling-pdr/inc/testing/z3pdr.h
badd +41 ~/Documents/master/pebbling-pdr/inc/algo/vpdr.h
badd +1 ~/Documents/master/pebbling-pdr/src/algo/pdr-logging.cpp
badd +113 ~/Documents/master/pebbling-pdr/src/algo/pdr.cpp
badd +306 ~/Documents/master/pebbling-pdr/src/cli-parse.cpp
badd +115 ~/Documents/master/pebbling-pdr/src/algo/ipdr-peter.cpp
badd +317 ~/Documents/master/pebbling-pdr/src/algo/ipdr-pebbling.cpp
badd +126 ~/Documents/master/pebbling-pdr/src/testing/z3ipdr.cpp
badd +46 ~/Documents/master/pebbling-pdr/src/testing/z3pebbling-experiments.cpp
badd +52 ~/Documents/master/pebbling-pdr/src/pebbling-pdr.cpp
badd +47 ~/Documents/master/pebbling-pdr/inc/auxiliary/types-ext.h
badd +39 ~/Documents/master/pebbling-pdr/src/model/pebbling/pebbling-model.cpp
badd +30 ~/Documents/master/pebbling-pdr/inc/tactic.h
badd +141 ~/Documents/master/pebbling-pdr/inc/cli-parse.h
badd +248 ~/Documents/master/pebbling-pdr/src/testing/experiments.cpp
badd +174 ~/Documents/master/pebbling-pdr/inc/testing/stats.h
badd +34 ~/Documents/master/pebbling-pdr/inc/model/pebbling/pebbling-model.h
badd +39 ~/Documents/master/pebbling-pdr/src/testing/z3-pebbling-model.cpp
badd +27 ~/Documents/master/pebbling-pdr/inc/auxiliary/z3-ext.h
badd +29 ~/Documents/master/pebbling-pdr/src/auxiliary/z3-ext.cpp
badd +1 ~/Documents/master/pebbling-pdr/inc/testing/z3-pebbling-experiments.h
badd +74 ~/Documents/master/pebbling-pdr/src/testing/pebbling-experiments.cpp
badd +42 ~/Documents/master/pebbling-pdr/inc/algo/result.h
badd +1 ~/Documents/master/pebbling-pdr/src/algo/result.cpp
badd +0 ~/Documents/master/pebbling-pdr/src/model/peterson/peterson-result.cpp
if exists('s:wipebuf') && len(win_findbuf(s:wipebuf)) == 0 && getbufvar(s:wipebuf, '&buftype') isnot# 'terminal'
  silent exe 'bwipe ' . s:wipebuf
endif
unlet! s:wipebuf
set winheight=1 winwidth=20 shortmess=filnxtToOFAcI
let s:sx = expand("<sfile>:p:r")."x.vim"
if filereadable(s:sx)
  exe "source " . fnameescape(s:sx)
endif
let &g:so = s:so_save | let &g:siso = s:siso_save
set hlsearch
nohlsearch
doautoall SessionLoadPost
unlet SessionLoad
" vim: set ft=vim :
