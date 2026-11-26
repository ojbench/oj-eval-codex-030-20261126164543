# Submission history

1. **879439**, commit `57e5c5b`: compile error, score 0. The provided Git
   submission mode sent the repository URL as literal header contents. The
   judge did not clone the repository. Added `--code-file` to the client to send
   the header contents through the same configured custom compiler.

2. **879445**, commit `7e7467a`: compile error, score 0. The judge's `IMAGE_T`
   definition conflicts with a typedef of that name. Removed the public type
   alias and added a regression check with `IMAGE_T` defined as a macro.
   A previous oversized API request was rejected without a submission ID;
   lossless run-length encoding reduced the source from 162 KB to 46 KB.
