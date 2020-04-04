pvrsrvkm-y += \
 server_breakpoint_bridge.o \
 server_cachegeneric_bridge.o \
 server_cmm_bridge.o \
 server_debugmisc_bridge.o \
 client_htbuffer_bridge.o \
 server_dmabuf_bridge.o \
 client_mm_bridge.o \
 client_pvrtl_bridge.o \
 client_sync_bridge.o \
 server_htbuffer_bridge.o \
 server_mm_bridge.o \
 server_pvrtl_bridge.o \
 server_regconfig_bridge.o \
 server_rgxcmp_bridge.o \
 server_rgxhwperf_bridge.o \
 server_rgxinit_bridge.o \
 server_rgxkicksync_bridge.o \
 server_rgxta3d_bridge.o \
 server_rgxtq_bridge.o \
 server_smm_bridge.o \
 server_srvcore_bridge.o \
 server_sync_bridge.o \
 server_syncsexport_bridge.o \
 server_timerquery_bridge.o \
 pvr_buffer_sync.o \
 pvr_fence.o \
 cache_generic.o \
 connection_server.o \
 devicemem_heapcfg.o \
 devicemem_server.o \
 handle.o \
 htbserver.o \
 lists.o \
 mmu_common.o \
 physheap.o \
 physmem.o \
 physmem_lma.o \
 physmem_tdsecbuf.o \
 pmr.o \
 power.o \
 process_stats.o \
 pvrsrv.o \
 srvcore.o \
 sync_server.o \
 tlintern.o \
 tlserver.o \
 tlstream.o \
 debugmisc_server.o \
 rgxbreakpoint.o \
 rgxccb.o \
 rgxcompute.o \
 rgxdebug.o \
 rgxfwutils.o \
 rgxhwperf.o \
 rgxinit.o \
 rgxkicksync.o \
 rgxlayer_km_impl.o \
 rgxmem.o \
 rgxmmuinit.o \
 rgxpower.o \
 rgxregconfig.o \
 rgxstartstop.o \
 rgxsync.o \
 rgxta3d.o \
 rgxtimecorr.o \
 rgxtimerquery.o \
 rgxtransfer.o \
 rgxutils.o \
 allocmem.o \
 devicemem_mmap_stub.o \
 event.o \
 handle_idr.o \
 mm.o \
 module_common.o \
 osconnection_server.o \
 osfunc.o \
 ossecure_export.o \
 pdump.o \
 physmem_dmabuf.o \
 physmem_osmem_linux.o \
 pmr_os.o \
 pvr_bridge_k.o \
 pvr_debug.o \
 pvr_debugfs.o \
 pvr_drm.o \
 pvr_dvfs_device.o \
 pvr_gputrace.o \
 pvr_hwperf.o \
 devicemem.o \
 devicemem_utils.o \
 hash.o \
 htbuffer.o \
 mem_utils.o \
 ra.o \
 sync.o \
 tlclient.o \
 uniq_key_splay_tree.o \
 rgx_compat_bvnc.o
pvrsrvkm-$(CONFIG_DRM_POWERVR_ROGUE_DEBUG) += \
 client_devicememhistory_bridge.o \
 server_devicememhistory_bridge.o \
 client_ri_bridge.o \
 server_ri_bridge.o \
 devicemem_history_server.o \
 ri_server.o
pvrsrvkm-$(CONFIG_DRM_POWERVR_ROGUE_KERNEL_SRVINIT) += \
 client_rgxinit_bridge.o \
 rgx_hwperf_table.o \
 htbinit.o \
 srvinit_pdump.o \
 rgxfwload.o \
 rgxfwimageutils.o \
 rgxlayer_impl.o \
 rgxsrvinit.o \
 rgxsrvinit_script.o \
 os_srvinit_param.o \
 srvinit_km.o
pvrsrvkm-$(CONFIG_DRM_POWERVR_ROGUE_PDUMP) += \
 client_pdumpmm_bridge.o \
 server_pdump_bridge.o \
 server_pdumpctrl_bridge.o \
 server_pdumpmm_bridge.o \
 server_rgxpdump_bridge.o \
 pdump_common.o \
 pdump_mmu.o \
 pdump_physmem.o \
 rgxpdump.o \
 devicemem_pdump.o \
 devicememx_pdump.o
pvrsrvkm-$(CONFIG_X86) += osfunc_x86.o
pvrsrvkm-$(CONFIG_ARM64) += osfunc_arm64.o
pvrsrvkm-$(CONFIG_EVENT_TRACING) += trace_events.o
