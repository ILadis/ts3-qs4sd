FROM ghcr.io/steamdeckhomebrew/holo-base:latest
RUN pacman -Sy --noconfirm gcc pkgconfig libpulse
ENTRYPOINT [ "/backend/entrypoint.sh" ]
