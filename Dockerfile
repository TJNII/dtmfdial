FROM debian:latest AS builder

RUN apt-get update && apt-get install -y build-essential

COPY ./ /dtmfdial

WORKDIR /dtmfdial

RUN useradd dtmf-build
RUN chown -R dtmf-build /dtmfdial

USER dtmf-build
RUN make

#
# output
#
FROM debian:latest

COPY --from=builder /dtmfdial /dtmfdial

RUN useradd -G audio dtmf
USER dtmf

ENTRYPOINT ["/dtmfdial/dial"]
