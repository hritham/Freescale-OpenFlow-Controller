include rules.mk
include $(cminfra_dir)/defs.system

all: buildall

#buildall: builducmcntrl buildcntrl builducmswitch buildswitch

switch: builducmswitch buildofswitch

controller: builducmcntrl buildcntrl

buildcntrl: 
	make -w -C $(cntrl_dir)

builducmcntrl:
	make -w -C $(cminfra_dir) -f makefile.cntrl

builducmswitch:
	make -w -C $(cminfra_dir) -f makefile.switch

buildofswitch:
	make -w -C $(switch_dir)

install:
	rm -rf /ondirector
	mkdir /ondirector
	mkdir /ondirector/bin
	mkdir /ondirector/debug
	mkdir /ondirector/conf
	mkdir /ondirector/app-conf
	mkdir /ondirector/certs
	mkdir /ondirector/html
	cp ondirector-start.sh /ondirector/
	cp ondirector-stop.sh /ondirector/
	cp -rf bin/lib  /ondirector/
	cp -f $(cminfra_dir)/obj/$(LIB_DIR)/*.so  /ondirector/lib
	cp -rf conf/on_director  /ondirector/conf
	cp -rf conf/cntrl-rconfig.xml  /ondirector/conf
	cp -rf src/ofcli/system/html/* /ondirector/html/
	cp -f bin/controller  /ondirector/bin/
	cp -f bin/ofcli  /ondirector/bin/
	cp -f bin/ofucmd  /ondirector/bin/
	cp -f bin/ofldsvd  /ondirector/bin/
	cp -f bin/httpd  /ondirector/bin/
	cp -f certs/sc-cacert.pem  /ondirector/certs/
	cp -f certs/sc2-cacert.pem  /ondirector/certs/
	cp -f certs/ctl-privkey.pem  /ondirector/certs/
	cp -f certs/ctl-cert.pem  /ondirector/certs/

image: controllerimage switchimage

controllerimage:
	mkdir ondirector-install
	mkdir ondirector-install/bin
	mkdir ondirector-install/conf
	mkdir ondirector-install/app-conf
	mkdir ondirector-install/certs
	mkdir ondirector-install/html
	cp ondirector-start.sh ondirector-install/
	cp ondirector-install.sh ondirector-install/
	cp ondirector-stop.sh ondirector-install/
	cp -rf bin  ondirector-install/
	cp -rf $(cminfra_dir)/obj/$(LIB_DIR)/*  ondirector-install/bin/lib
	cp -rf certs  ondirector-install/
	cp -rf conf ondirector-install/
	cp -rf src/ofcli/system/html/* ondirector-install/html/
	tar czf ondirector-image.tar.gz ondirector-install
	rm -rf ondirector-install

switchimage:
	mkdir onswitch
	mkdir onswitch/bin
	mkdir onswitch/conf
	mkdir onswitch/app-conf
	mkdir onswitch/certs
	mkdir onswitch/html
	cp onswitch-start.sh onswitch/
	cp switch-install.sh onswitch/
	cp onswitch-stop.sh onswitch/
	cp -rf bin  onswitch/
	cp -rf $(cminfra_dir)/obj/$(LIB_DIR)/*  onswitch/bin/lib
	cp -rf certs onswitch/
	cp -rf conf onswitch/
	cp -rf switch/mgmt/system/html/* onswitch/html/
	tar czf switch-image.tar.gz onswitch
	rm -rf onswitch
	make -w -C $(switch_dir) release
ifeq ($(TARGET_PLATFORM),p2041rdb)
	cp -rf $(switch_dir)/.p2041rdb-sdk1.3/_release/OFSwitchImage.sh .
endif
ifeq ($(TARGET_PLATFORM),p4080ds)
	cp -rf $(switch_dir)/.p4080ds-sdk1.3/_release/OFSwitchImage.sh .
endif
	tar czf onswitch-image.tar.gz OFSwitchImage.sh switch-image.tar.gz onswitch-install.sh
	rm -f OFSwitchImage.sh 

clean: cleanall

cleanall: cleanucmcntrl cleancntrl cleanucmswitch cleanswitchobj

cleancontroller: cleanucmcntrl cleancntrl

cleanswitch: cleanucmswitch cleanswitchobj

cleanucmcntrl:
	make -w -C $(cminfra_dir) -f makefile.cntrl clean
	rm -rf cntrl-image.tar.gz

cleanucmswitch:
	make -w -C $(cminfra_dir) -f makefile.switch clean
	rm -rf switch-image.tar.gz

cleancntrl:
	make -w -C $(cntrl_dir) clean

cleanswitchobj:
	make -w -C $(switch_dir) clean

