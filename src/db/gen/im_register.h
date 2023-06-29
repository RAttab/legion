/* This file was generated by ./legion --db */
#pragma once


// -----------------------------------------------------------------------------
// natural
// -----------------------------------------------------------------------------

im_register(item_elem_a, "elem-a", 6, "item-elem-a"),
im_register(item_elem_b, "elem-b", 6, "item-elem-b"),
im_register(item_elem_c, "elem-c", 6, "item-elem-c"),
im_register(item_elem_d, "elem-d", 6, "item-elem-d"),
im_register(item_elem_e, "elem-e", 6, "item-elem-e"),
im_register(item_elem_f, "elem-f", 6, "item-elem-f"),
im_register(item_elem_g, "elem-g", 6, "item-elem-g"),
im_register(item_elem_h, "elem-h", 6, "item-elem-h"),
im_register(item_elem_i, "elem-i", 6, "item-elem-i"),
im_register(item_elem_j, "elem-j", 6, "item-elem-j"),
im_register(item_elem_k, "elem-k", 6, "item-elem-k"),

// -----------------------------------------------------------------------------
// synth
// -----------------------------------------------------------------------------

im_register(item_elem_l, "elem-l", 6, "item-elem-l"),
im_register(item_elem_m, "elem-m", 6, "item-elem-m"),
im_register(item_elem_n, "elem-n", 6, "item-elem-n"),
im_register(item_elem_o, "elem-o", 6, "item-elem-o"),
im_register(item_elem_p, "elem-p", 6, "item-elem-p"),
im_register(item_elem_q, "elem-q", 6, "item-elem-q"),
im_register(item_elem_r, "elem-r", 6, "item-elem-r"),
im_register(item_elem_s, "elem-s", 6, "item-elem-s"),
im_register(item_elem_t, "elem-t", 6, "item-elem-t"),
im_register(item_elem_u, "elem-u", 6, "item-elem-u"),
im_register(item_elem_v, "elem-v", 6, "item-elem-v"),
im_register(item_elem_w, "elem-w", 6, "item-elem-w"),
im_register(item_elem_x, "elem-x", 6, "item-elem-x"),
im_register(item_elem_y, "elem-y", 6, "item-elem-y"),
im_register(item_elem_z, "elem-z", 6, "item-elem-z"),

// -----------------------------------------------------------------------------
// logistics
// -----------------------------------------------------------------------------

im_register(item_rod, "rod", 3, "item-rod"),
im_register(item_solar, "solar", 5, "item-solar"),
im_register(item_worker, "worker", 6, "item-worker"),
im_register(item_battery, "battery", 7, "item-battery"),
im_register(item_accelerator, "accelerator", 11, "item-accelerator"),
im_register(item_pill, "pill", 4, "item-pill"),
im_register(item_kwheel, "kwheel", 6, "item-kwheel"),

// -----------------------------------------------------------------------------
// active
// -----------------------------------------------------------------------------

im_register_cfg(item_extract, "extract", 7, "item-extract", im_extract_config),
im_register_cfg(item_printer, "printer", 7, "item-printer", im_printer_config),
im_register_cfg(item_deploy, "deploy", 6, "item-deploy", im_deploy_config),
im_register_cfg(item_assembly, "assembly", 8, "item-assembly", im_printer_config),
im_register_cfg(item_fusion, "fusion", 6, "item-fusion", im_fusion_config),
im_register_cfg(item_memory, "memory", 6, "item-memory", im_memory_config),
im_register_cfg(item_storage, "storage", 7, "item-storage", im_storage_config),
im_register_cfg(item_brain, "brain", 5, "item-brain", im_brain_config),
im_register_cfg(item_library, "library", 7, "item-library", im_library_config),
im_register_cfg(item_prober, "prober", 6, "item-prober", im_prober_config),
im_register_cfg(item_receive, "receive", 7, "item-receive", im_receive_config),
im_register_cfg(item_burner, "burner", 6, "item-burner", im_burner_config),
im_register_cfg(item_condenser, "condenser", 9, "item-condenser", im_extract_config),
im_register_cfg(item_lab, "lab", 3, "item-lab", im_lab_config),
im_register_cfg(item_port, "port", 4, "item-port", im_port_config),
im_register_cfg(item_scanner, "scanner", 7, "item-scanner", im_scanner_config),
im_register_cfg(item_transmit, "transmit", 8, "item-transmit", im_transmit_config),
im_register_cfg(item_collider, "collider", 8, "item-collider", im_collider_config),
im_register_cfg(item_legion, "legion", 6, "item-legion", im_legion_config),
im_register_cfg(item_packer, "packer", 6, "item-packer", im_packer_config),
im_register_cfg(item_nomad, "nomad", 5, "item-nomad", im_nomad_config),
im_register_cfg(item_test, "test", 4, "item-test", im_test_config),

// -----------------------------------------------------------------------------
// passive
// -----------------------------------------------------------------------------

im_register(item_monarkols, "monarkols", 9, "item-monarkols"),
im_register(item_monobararkon, "monobararkon", 12, "item-monobararkon"),
im_register(item_monobarex, "monobarex", 9, "item-monobarex"),
im_register(item_monobarols, "monobarols", 10, "item-monobarols"),
im_register(item_monocharkoid, "monocharkoid", 12, "item-monocharkoid"),
im_register(item_monochate, "monochate", 9, "item-monochate"),
im_register(item_monochubaride, "monochubaride", 13, "item-monochubaride"),
im_register(item_monochury, "monochury", 9, "item-monochury"),
im_register(item_duarksh, "duarksh", 7, "item-duarksh"),
im_register(item_duerldylon_monochols, "duerldylon-monochols", 20, "item-duerldylon-monochols"),
im_register(item_duerlex, "duerlex", 7, "item-duerlex"),
im_register(item_duerlry, "duerlry", 7, "item-duerlry"),
im_register(item_duerltor, "duerltor", 8, "item-duerltor"),
im_register(item_duochium, "duochium", 8, "item-duochium"),
im_register(item_duodylchalt_monobarols, "duodylchalt-monobarols", 22, "item-duodylchalt-monobarols"),
im_register(item_duodylitil, "duodylitil", 10, "item-duodylitil"),
im_register(item_duodylium, "duodylium", 9, "item-duodylium"),
im_register(item_trerlchury_duobargen, "trerlchury-duobargen", 20, "item-trerlchury-duobargen"),
im_register(item_trichubarium, "trichubarium", 12, "item-trichubarium"),
im_register(item_tridylarkitil, "tridylarkitil", 13, "item-tridylarkitil"),
im_register(item_tridylate, "tridylate", 9, "item-tridylate"),
im_register(item_tridylgen, "tridylgen", 9, "item-tridylgen"),
im_register(item_trifimate, "trifimate", 9, "item-trifimate"),
im_register(item_trifimbarsh, "trifimbarsh", 11, "item-trifimbarsh"),
im_register(item_trifimium, "trifimium", 9, "item-trifimium"),
im_register(item_tetradylchitil_duobarate, "tetradylchitil-duobarate", 24, "item-tetradylchitil-duobarate"),
im_register(item_tetradylgen, "tetradylgen", 11, "item-tetradylgen"),
im_register(item_tetrafimalm, "tetrafimalm", 11, "item-tetrafimalm"),
im_register(item_tetrafimalt, "tetrafimalt", 11, "item-tetrafimalt"),
im_register(item_tetrafimry, "tetrafimry", 10, "item-tetrafimry"),
im_register(item_tetrerlbargen, "tetrerlbargen", 13, "item-tetrerlbargen"),
im_register(item_pentadylchate, "pentadylchate", 13, "item-pentadylchate"),
im_register(item_pentadylchutor, "pentadylchutor", 14, "item-pentadylchutor"),
im_register(item_pentafimchex_monobarsh, "pentafimchex-monobarsh", 22, "item-pentafimchex-monobarsh"),
im_register(item_pentafimry, "pentafimry", 10, "item-pentafimry"),
im_register(item_pentalofchols, "pentalofchols", 13, "item-pentalofchols"),
im_register(item_pentamoxate, "pentamoxate", 11, "item-pentamoxate"),
im_register(item_penterltor, "penterltor", 10, "item-penterltor"),
im_register(item_hexadylchate_pentabaron, "hexadylchate-pentabaron", 23, "item-hexadylchate-pentabaron"),
im_register(item_hexamoxchoid_monobary, "hexamoxchoid-monobary", 21, "item-hexamoxchoid-monobary"),

// -----------------------------------------------------------------------------
// sys
// -----------------------------------------------------------------------------

im_register(item_data, "data", 4, "item-data"),
im_register(item_dummy, "dummy", 5, "item-dummy"),
im_register(item_energy, "energy", 6, "item-energy"),
im_register(item_user, "user", 4, "item-user"),
