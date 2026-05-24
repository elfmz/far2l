sed -i "s|FAR2L_VERSION = .*|FAR2L_VERSION = $NEW_TAG|" buildroot/far2l/far2l.mk
git add ./buildroot/far2l/far2l.mk
