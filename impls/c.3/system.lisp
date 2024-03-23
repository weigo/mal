(def! not
    (fn* (a)
	 (if a false true)))

(def! load-file
    (fn* (f)
	 (eval (read-string (str "(do " (slurp f) "\nnil)")))))

(defmacro! cond
    (fn* (& xs)
	 (if (> (count xs) 0)
	     (list 'if (first xs)
		   (if (> (count xs) 1)
		       (nth xs 1)
		       (throw "odd number of forms to cond"))
		   (cons 'cond (rest (rest xs)))))))

(def! in-package
    (fn* (package-name)
	 (let* (package (find-package package-name))
	   (if (package? package)
	       (def! *PACKAGE* package)
	       (throw "package not found"))
	   )))

;(defpackage package-name
;  (:use :package-a "package-b" 'package-c)
;  (:nicknames :alias-a "alias-b" 'alias-c)
;  (:export 'symbol-a "symbol-b"))
