((nil . ((indent-tabs-mode . nil)
         (tab-width . 3)
         (c-basic-offset . 3)
         (c-ts-mode-indent-offset . 3)
         (fill-column . 80)
         (compile-command . "just build")
         (eval . (setq-local lsp-clients-clangd-executable
                             (expand-file-name
                              ".cache/cat-llvm/bin/clangd"
                              (locate-dominating-file default-directory
                                                      ".dir-locals.el")))))))
