FROM ghcr.io/steamdeckhomebrew/holo-base:latest
RUN pacman -Sy --noconfirm gcc pkgconfig util-linux-libs sqlite libpulse
ENTRYPOINT [ "/backend/entrypoint.sh" ]
