" This is the Vim filetype detect script for Blaze.
" put it into you ~/.vim/ftdetect/

augroup filetype
    autocmd! BufRead,BufNewFile BUILD setfiletype blade
augroup end

