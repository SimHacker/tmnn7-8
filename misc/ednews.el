;; ednews.el -- package for structured communication with the ednews back end
;;
;; ednews.el 1.0 was written by Eric S. Raymond (eric@snark) 22 Oct 1987
;;
;; This package is written for use with the ednews server. It is
;; intended to be used for writing custom mail readers in GNU Emacs.
;; It is known to work OK under GNU Emacs 18.47 on a 3B1.
;;
;; There are three main entry points and three hooks; ednews, ednews-send,
;; ednews-message-hook ednews-command-hook and ednews-response-hook. There
;; are also a number of state variables that are updated by each server
;; response. An ednews-kill function is also provided.
;;
;; The ednews-start function spawns a copy of ednews and initially sets the
;; state variables. The ednews-send function sends a string down to
;; the ednews and accepts a response, updating the state variables.
;;
;; The message hook gets called on each information message as it comes in.
;; The command hook gets called after each command and state update.
;; The response hook gets called after each response sequence ends.
;; A BYE response automatically deletes the ednews process.
;;
;; The state variables keep all current information on the current article
;; location and ednews state, as revealed by its responses. For more info
;; on the message protocol itself, see ednews(1) in the news documentation.
;;
;; A test function, ednews-test-command-hook, is provided. It is the default
;; value of command-hook; if you do (ednews-start) without setting the latter
;; you will find yourself running a simple ednews emulator. You must do
;; ednews-send calls to send it commands.

(provide 'ednews)

;; These variables are updated on each command cycle:

(defvar ednews-last-command nil
"An atom giving the type of the last message received from ednews. One of
the following:
      ednews-message	-- an informational message
      ednews-topics	-- a TOPICS response
      ednews-group	-- a GROUP response
      ednews-article	-- an ARTICLE response
      ednews-subjects	-- a SUBJECTS response
      ednews-done	-- a DONE response
      ednews-nomore	-- a NOMORE response
      ednews-cmderr	-- a CMDERR response
      ednews-error	-- there was an I/O error on the last response
")
(defvar ednews-trace nil
"A list (in reverse order) of the message types received in the last response")

(defvar ednews-last-message nil "If non-nil, the body of the last message")
(defvar ednews-status nil "A list of ednews status attributes")

(defvar topic-file nil "The location of the current topics file")
(defvar topic-waiting 0 "The total number of messages waiting on this pass")

(defvar group-name nil "The name of the current group")
(defvar group-least 0 "The lowest active article # for the current group")
(defvar group-greatest 0 "The highest active article # in the current group")
(defvar group-unread 0 "The number of unread messages remaining in the group")

(defvar article-file nil "The location of the current article file")
(defvar article-lines 0 "The number of lines in the current article")
(defvar article-seen nil "T if current article has been seen, nil if not")

(defvar subject-file nil "Name of current subject list file")

(defvar cmderr-message nil "The error message from the last invalid command")

;; Hook functions

(defvar ednews-buffer "*ednews*")

(defvar ednews-message-hook 'message
  "If non-nil, a function name to be applied to each information message")

(defvar ednews-command-hook 'ednews-test-command-hook
  "If non-nil, a function name to be evaluated after each command response")

(defvar ednews-response-hook 'ednews-test-response-hook
  "If non-nil, a function name to be evaluated after each response sequence")

;; Public state variables end here

(defun ednews-start ()
;; Start an ednews server, if there isn't already one going
  (if (get-process "ednews") nil
    (let ((ednews (start-process "ednews" ednews-buffer "ednews")))
      (pop-to-buffer ednews-buffer)
      (set-process-filter ednews 'ednews-daemon)
      (ednews-send nil)
      t
      )
  )
)

(defun ednews-daemon (process msgline)
;; process each physical line of a response as it comes up 
  (while (string-match "\\(.*\\)\n" msgline)
    (let ((endm (match-end 1)))
      (ednews-procline (substring msgline 0 endm))
      (setq msgline (substring msgline (+ endm 1)))
      )
    )
  )

(defun ednews-procline (msgline)
;; process each logical line of a response as it comes up 
  (if (not (equal (substring msgline 0 1) "\007"))
      (ednews-message-imp msgline)
    (let ((cmdchar (substring msgline 1 2)))
     (cond
      ((equal cmdchar "T")	(ednews-topics-imp (substring msgline 7)))
      ((equal cmdchar "G")	(ednews-group-imp (substring msgline 6)))
      ((equal cmdchar "A")	(ednews-article-imp (substring msgline 8)))
      ((equal cmdchar "S")	(ednews-subject-imp (substring msgline 9)))
      ((equal cmdchar "D")	(ednews-done-imp (substring msgline 5)))
      ((equal cmdchar "N")	(setq ednews-last-command 'ednews-nomore))
      ((equal cmdchar "B")	(ednews-kill))
      ((equal cmdchar "C")	(ednews-cmderr-imp (substring msgline 7)))
      (t			(setq ednews-last-command 'ednews-error))
      )
     )
    )
  (if ednews-command-hook (apply ednews-command-hook nil))
  (append ednews-trace (list ednews-last-command))
  )

(defun ednews-message-imp (str)
;; handle information message
  (setq ednews-last-command 'ednews-message)
  (setq ednews-last-message
	(if (equal (substring str -1) "\n")
	    (substring str 0 -1)
	  str)
	)
  (if ednews-message-hook (apply ednews-message-hook ednews-last-message nil))
)

(defun ednews-topics-imp (str)
;; handle TOPICS message
  (setq ednews-last-command 'ednews-topics)
  (string-match " *\\([^ ]*\\) \\([0-9]*\\)" str)
  (setq topic-file (substring str (match-beginning 1) (match-end 1)))
  (setq topic-waiting (substring str (match-beginning 2) (match-end 2)))
  )

(defun ednews-group-imp (str)
;; handle GROUP message
  (setq ednews-last-command 'ednews-group)
  (string-match " *\\([^ ]*\\) \\([0-9]*\\) \\([0-9]*\\) \\([0-9]*\\)" str)
  (setq group-name (substring str (match-beginning 1) (match-end 1)))
  (setq group-least (substring str (match-beginning 2) (match-end 2)))
  (setq group-greatest (substring str (match-beginning 3) (match-end 3)))
  (setq group-unread (substring str (match-beginning 4) (match-end 4)))
  )

(defun ednews-article-imp (str)
;; handle ARTICLE message
  (string-match " *\\([^ ]*\\) \\([0-9]*\\) \\([a-z]*\\)" str)
  (setq ednews-last-command 'ednews-article)
  (setq article-file (substring str (match-beginning 1) (match-end 1)))
  (setq article-lines (substring str (match-beginning 2) (match-end 2)))
  (setq article-seen
	(equal (substring str (match-beginning 3) (match-end 3)) "seen"))
  )

(defun ednews-subjects-imp (str)
;; handle SUBJECTS message
  (setq ednews-last-command 'ednews-subjects)
  (setq subject-file str)
)

(defun ednews-done-imp (str)
;; handle DONE message
  (setq ednews-last-command 'ednews-done)
  (setq ednews-status (cdr (car (read-from-string (concat "(" str ")")))))
)

(defun ednews-kill ()
;; handle BYE message, also be an entry point for aborting the ednews
  (setq ednews-last-command nil)
  (let ((ednews (get-process "ednews")))
	(if ednews (kill-process ednews)))
  (kill-buffer ednews-buffer)
)

(defun ednews-cmderr-imp (str)
;; handle CMDERR message
  (setq ednews-last-command 'ednews-cmderr)
  (setq cmderr-message str)
)

(defun ednews-send (str)
;; send a command down to the server, accept a response
  (setq ednews-trace nil)
  (setq ednews-last-command nil)
  (let ((ednews (get-process "ednews")))
    (if str (send-string ednews (concat str "\n")))
    (while 
	(and
	  (accept-process-output ednews)
	  (not (memq ednews-last-command
		   '(ednews-article ednews-nomore ednews-cmderr ednews-bye)))
	  )
      )
    (if ednews-last-command
	(if ednews-response-hook (apply ednews-response-hook nil)))
    )
  )

(defun ednews-test-command-hook ()
  (cond
   ((eq ednews-last-command 'ednews-message)
    (insert "Message: " ednews-last-message "\n"))
   ((eq ednews-last-command 'ednews-topics)
    (insert "TOPICS: " topic-file " " topic-waiting "\n"))
   ((eq ednews-last-command 'ednews-group)
    (insert "GROUP: " group-name
	    " " group-least " " group-greatest " " group-unread "\n"))
   ((eq ednews-last-command 'ednews-article)
    (insert "ARTICLE: " article-file
	    " " article-lines " " (if article-seen "seen" "unseen") "\n"))
   ((eq ednews-last-command 'ednews-subjects)
    (insert "SUBJECTS: " subjects-file "\n"))
   ((eq ednews-last-command 'ednews-done)
    (insert "DONE: ")
    (princ ednews-status 'insert)
    (insert "\n"))
   ((eq ednews-last-command 'ednews-nomore)
    (insert "NOMORE\n"))
   ((eq ednews-last-command 'ednews-cmderr)
    (insert "CMDERR: " cmderr-message "\n"))
   ((eq ednews-last-command 'ednews-bye)
    (insert "BYE\n"))
   ((eq ednews-last-command 'ednews-error)
    (insert "I/O Error\n"))
   (t
    (insert "I'm confused\n"))
   )
  )


(defun ednews-test-response-hook ()
  (message "Please send down an ednews command")
  )

;; ednews.el ends here
