include ../../rules.mk

all: build

makesampleapp: 
	make -w -C $(ofapps_dir)/sampleapps
#	make -w -C $(ofapps_dir)/netlink_listener_app
#	make -w -C $(ofapps_dir)/arp_nf_bkend
#	make -w -C $(ofapps_dir)/ipfwd_nf_bkend

makextensionapps: 
#	make -w -C $(ofapps_dir)/extensions

build: makesampleapp makextensionapps

clean:
	make -C $(ofapps_dir)/sampleapps clean
#	make -C $(ofapps_dir)/extensions clean
#	make -C $(ofapps_dir)/netlink_listener_app clean
#	make -C $(ofapps_dir)/arp_nf_bkend clean
#	make -C $(ofapps_dir)/ipfwd_nf_bkend clean
