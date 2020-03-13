FROM debian:stretch

RUN apt-get update && apt-get install -y build-essential

# DEBUG
RUN apt-get install -y emacs-nox

COPY ./ /dtmfdial

WORKDIR /dtmfdial

RUN make

RUN useradd -G audio dtmf
USER dtmf

ENTRYPOINT ["/dtmfdial/dial"]
