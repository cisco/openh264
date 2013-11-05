OpenH264
========

This goal of this project is to build a BSD-licensed open source H.264
codec that is suitable for use in real time applications such as WebRTC. See
http://www.openh264.org/ for more details.

This repo has no codec code yet (only the code for the website, in the gh-pages
branch). The code exists, and is used internally in Cisco products. But before
we can release it and start the public project, we need to do the following
things:

* Separate the code from its dependencies on Cisco source code which is not
  intended to be open sourced;

* Check that we won't 0-day other Cisco products by releasing code which
  has known unpublished security vulnerabilities;

* Make sure all the legal processes necessary before opening code have been
  completed.

We hope to have these steps completed soon.
