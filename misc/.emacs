(custom-set-variables
  ;; custom-set-variables was added by Custom.
  ;; If you edit it by hand, you could mess it up, so be careful.
  ;; Your init file should contain only one such instance.
  ;; If there is more than one, they won't work right.
 )
(custom-set-faces
  ;; custom-set-faces was added by Custom.
  ;; If you edit it by hand, you could mess it up, so be careful.
  ;; Your init file should contain only one such instance.
  ;; If there is more than one, they won't work right.
 '(default ((t (:inherit nil :stipple nil :background "SystemWindow" :foreground "SystemWindowText" :inverse-video nil :box nil :strike-through nil :overline nil :underline nil :slant normal :weight normal :height 98 :width normal :foundry "outline" :family "Courier New")))))

(add-to-list 'auto-mode-alist '("\\.h\\'" . c++-mode))

(load-library "view")
(require 'cc-mode)
(require 'ido)

(ido-mode t)

(defun my-c-mode-common-hook()

(setq-default indent-tabs-mode nil)
(setq tab-width 4)
;(defvaralias 'c-basic-offset 'tab-width)
(setq c-tab-always-indent t)

; No offset on braces
(c-set-offset 'substatement-open 0)
; Offset on case
(c-set-offset 'case-label 4)
; No offset on case braces
(c-set-offset 'statement-case-open 0)
(c-set-offset 'statement-case-intro 0)
(c-set-offset 'statement-block-intro 4)
(c-set-offset 'defun-block-intro 4)
(c-set-offset 'inclass '4)
)

;(defun my-c-mode-common-hook()

;(c-set-offset 'substatement-open 0)

;(setq c++-tab-always-indent t)
;(setq c-basic-offset 4)
;(setq c-indent-level 4)

;(setq tab-stop-list '(4 8 12 16 20 24 28 32 36 40 44 48 52 56 60))
;(setq tab-width 4)
;(setq indent-tabs-mode t)
;)

(add-hook 'c-mode-common-hook 'my-c-mode-common-hook)

; Indents on newline when hit return
(define-key c++-mode-map (kbd "RET") 'newline-and-indent)

(split-window-horizontally)

; Autocomplete on Tab
(define-key c++-mode-map "\t" 'dabbrev-expand)

; Shift-Tab for indentation
(define-key c++-mode-map [S-tab] 'indent-for-tab-command)

; Turn off toolbar
(tool-bar-mode 0)

(scroll-bar-mode -1)

(global-hl-line-mode 1)
(set-face-background 'hl-line "midnight blue")

(add-to-list 'default-frame-alist '(font . "Liberation Mono-11.5"))
(set-face-attribute 'default t :font "Liberation Mono-11.5")
(set-face-attribute 'font-lock-doc-face nil :foreground "gray50")
(set-face-attribute 'font-lock-builtin-face nil :foreground "#DAB98F")
(set-face-attribute 'font-lock-comment-face nil :foreground "gray50")
(set-face-attribute 'font-lock-constant-face nil :foreground "olive drab")
(set-face-attribute 'font-lock-function-name-face nil :foreground "burlywood3")
(set-face-attribute 'font-lock-string-face nil :foreground "olive drab")
(set-face-attribute 'font-lock-keyword-face nil :foreground "DarkGoldenrod3")
(set-face-attribute 'font-lock-type-face nil :foreground "burlywood3")
(set-face-attribute 'font-lock-variable-name-face nil :foreground "burlywood3")

; Switching buffer
(global-set-key (read-kbd-macro "\eb") 'ido-switch-buffer)
(global-set-key (read-kbd-macro "\eB") 'ido-switch-buffer-other-window)

; Find files
(define-key global-map "\ef" 'find-file)
(define-key global-map "\eF" 'find-file-other-window)

; Buffer
(define-key global-map "\er" 'revert-buffer)
(define-key global-map "\ek" 'kill-this-buffer)
(define-key global-map "\es" 'save-buffer)

(define-key global-map [C-tab] 'indent-region)

(define-key global-map "\ew" 'other-window)

(define-key c++-mode-map "\e/" 'c-mark-function)

(define-key global-map "\eg" 'goto-line)
(define-key c++-mode-map "\ej" 'imenu)

; Editting
(define-key global-map "" 'copy-region-as-kill)
(define-key global-map "" 'yank)
(define-key global-map "\eu" 'undo)

(define-key global-map "\eo" 'query-replace)


(defun post-load-stuff()
  (menu-bar-mode -1)
  ; maximized
  (w32-send-sys-command 61488)
  (set-background-color "#161616")
  (set-foreground-color "burlywood3")
  (set-cursor-color "#40FF40")
)
(add-hook 'window-setup-hook 'post-load-stuff t)

; No auto-save
(setq auto-save-default nil)

; Backups in one place
(setq
 backup-by-copying t
 backup-directory-alist '(("." . "~/.saves"))
 delete-old-versions t
 kept-new-versions 6
 kept-old-versions 2
 version-control nil)


(defun find-corresponding-file ()
  "Find the file that corresponds to this one."
  (interactive)
  (setq corresponding_filename nil)
  (setq base_filename (file-name-sans-extension buffer-file-name))
  (if (string-match "\\.c" buffer-file-name)
      (setq corresponding_filename (concat base_filename ".h")))
  (if (string-match "\\.h" buffer-file-name)
      (if (file-exists-p (concat base_filename ".c")) (setq corresponding_filename (concat base_filename ".c"))
      (setq corresponding_filename (concat base_filename ".cpp"))))
  (if (string-match "\\.cpp" buffer-file-name)
      (setq corresponding_filename (concat base_filename ".h")))
  (if corresponding_filename (find-file corresponding_filename)
    (error "Unable to find a corresponding file")))

(defun find-corresponding-file-other-window ()
  "Find the file that corresponds to this one."
  (interactive)
  (find-file-other-window buffer-file-name)
  (find-corresponding-file)
  (other-window -1))

(define-key c++-mode-map "\ec" 'find-corresponding-file)
(define-key c++-mode-map "\eC" 'find-corresponding-file-other-window)

; Building
(setq makescript "build.bat")
(setq compilation-directory-locked nil)

(defun find-project-directory-recursive ()
  "Recursively search for a makefile."
  (interactive)
  (if (file-exists-p makescript) t
      (cd "../")
      (find-project-directory-recursive)))

(defun find-project-directory ()
  "Find the project directory."
  (interactive)
  (setq find-project-from-directory default-directory)
  (switch-to-buffer-other-window "*compilation*")
  (if compilation-directory-locked (cd last-compilation-directory)
    (cd find-project-from-directory)
    (find-project-directory-recursive)
    (setq last-compilation-directory default-directory)))

(defun make-without-asking ()
  "Make the current build."
  (interactive)
  (if (find-project-directory) (compile makescript))
  (other-window 1))

(define-key global-map "\em" 'make-without-asking)

(define-key global-map "\en" 'next-error)
(define-key global-map "\eN" 'previous-error)

(define-key global-map "\e:" 'View-back-to-mark)

(define-key global-map "\e[" 'start-kbd-macro)
(define-key global-map "\e]" 'end-kbd-macro)
(define-key global-map "\e'" 'call-last-kbd-macro)