/* This file is generated by ./legion --code */


tape_register(item_muscle, 4, {
  .id = item_muscle
  .host = item_printer
  .work = 2
  .energy = 2
  .inputs = 3
  .outputs = 1
  .tape = {
    [000] = item_elem_a,
    [001] = item_elem_a,
    [002] = item_elem_a,
    [003] = item_muscle,
  }
});

tape_register(item_nodule, 5, {
  .id = item_nodule
  .host = item_printer
  .work = 2
  .energy = 2
  .inputs = 4
  .outputs = 1
  .tape = {
    [000] = item_elem_a,
    [001] = item_elem_b,
    [002] = item_elem_b,
    [003] = item_elem_a,
    [004] = item_nodule,
  }
});

tape_register(item_vein, 4, {
  .id = item_vein
  .host = item_printer
  .work = 2
  .energy = 2
  .inputs = 3
  .outputs = 1
  .tape = {
    [000] = item_elem_a,
    [001] = item_elem_c,
    [002] = item_elem_a,
    [003] = item_vein,
  }
});

tape_register(item_bone, 6, {
  .id = item_bone
  .host = item_printer
  .work = 2
  .energy = 2
  .inputs = 5
  .outputs = 1
  .tape = {
    [000] = item_elem_b,
    [001] = item_elem_b,
    [002] = item_elem_b,
    [003] = item_elem_b,
    [004] = item_elem_b,
    [005] = item_bone,
  }
});

tape_register(item_tendon, 6, {
  .id = item_tendon
  .host = item_printer
  .work = 4
  .energy = 2
  .inputs = 5
  .outputs = 1
  .tape = {
    [000] = item_elem_b,
    [001] = item_elem_a,
    [002] = item_elem_a,
    [003] = item_elem_a,
    [004] = item_elem_b,
    [005] = item_tendon,
  }
});

tape_register(item_rod, 14, {
  .id = item_rod
  .host = item_printer
  .work = 8
  .energy = 8
  .inputs = 13
  .outputs = 1
  .tape = {
    [000] = item_elem_b,
    [001] = item_elem_a,
    [002] = item_elem_a,
    [003] = item_elem_a,
    [004] = item_elem_a,
    [005] = item_elem_a,
    [006] = item_elem_b,
    [007] = item_elem_a,
    [008] = item_elem_a,
    [009] = item_elem_a,
    [010] = item_elem_a,
    [011] = item_elem_a,
    [012] = item_elem_b,
    [013] = item_rod,
  }
});

tape_register(item_torus, 9, {
  .id = item_torus
  .host = item_printer
  .work = 4
  .energy = 4
  .inputs = 8
  .outputs = 1
  .tape = {
    [000] = item_elem_b,
    [001] = item_elem_a,
    [002] = item_elem_a,
    [003] = item_elem_b,
    [004] = item_elem_b,
    [005] = item_elem_a,
    [006] = item_elem_a,
    [007] = item_elem_b,
    [008] = item_torus,
  }
});

tape_register(item_lens, 5, {
  .id = item_lens
  .host = item_printer
  .work = 2
  .energy = 4
  .inputs = 4
  .outputs = 1
  .tape = {
    [000] = item_elem_b,
    [001] = item_elem_c,
    [002] = item_elem_c,
    [003] = item_elem_b,
    [004] = item_lens,
  }
});

tape_register(item_nerve, 4, {
  .id = item_nerve
  .host = item_printer
  .work = 2
  .energy = 4
  .inputs = 3
  .outputs = 1
  .tape = {
    [000] = item_elem_c,
    [001] = item_elem_c,
    [002] = item_elem_c,
    [003] = item_nerve,
  }
});

tape_register(item_neuron, 6, {
  .id = item_neuron
  .host = item_printer
  .work = 4
  .energy = 4
  .inputs = 5
  .outputs = 1
  .tape = {
    [000] = item_elem_c,
    [001] = item_elem_a,
    [002] = item_elem_c,
    [003] = item_elem_a,
    [004] = item_elem_c,
    [005] = item_neuron,
  }
});

tape_register(item_retina, 5, {
  .id = item_retina
  .host = item_printer
  .work = 4
  .energy = 4
  .inputs = 4
  .outputs = 1
  .tape = {
    [000] = item_elem_c,
    [001] = item_elem_b,
    [002] = item_elem_b,
    [003] = item_elem_c,
    [004] = item_retina,
  }
});

tape_register(item_magnet, 7, {
  .id = item_magnet
  .host = item_printer
  .work = 8
  .energy = 16
  .inputs = 6
  .outputs = 1
  .tape = {
    [000] = item_elem_d,
    [001] = item_elem_d,
    [002] = item_elem_b,
    [003] = item_elem_b,
    [004] = item_elem_d,
    [005] = item_elem_d,
    [006] = item_magnet,
  }
});

tape_register(item_ferrofluid, 6, {
  .id = item_ferrofluid
  .host = item_printer
  .work = 8
  .energy = 16
  .inputs = 5
  .outputs = 1
  .tape = {
    [000] = item_elem_d,
    [001] = item_elem_c,
    [002] = item_elem_d,
    [003] = item_elem_c,
    [004] = item_elem_d,
    [005] = item_ferrofluid,
  }
});

tape_register(item_semiconductor, 9, {
  .id = item_semiconductor
  .host = item_printer
  .work = 10
  .energy = 24
  .inputs = 8
  .outputs = 1
  .tape = {
    [000] = item_elem_d,
    [001] = item_elem_d,
    [002] = item_elem_a,
    [003] = item_elem_c,
    [004] = item_elem_c,
    [005] = item_elem_a,
    [006] = item_elem_d,
    [007] = item_elem_d,
    [008] = item_semiconductor,
  }
});

tape_register(item_conductor, 9, {
  .id = item_conductor
  .host = item_printer
  .work = 10
  .energy = 24
  .inputs = 8
  .outputs = 1
  .tape = {
    [000] = item_elem_g,
    [001] = item_elem_g,
    [002] = item_elem_g,
    [003] = item_elem_d,
    [004] = item_elem_d,
    [005] = item_elem_g,
    [006] = item_elem_g,
    [007] = item_elem_g,
    [008] = item_conductor,
  }
});

tape_register(item_galvanic, 9, {
  .id = item_galvanic
  .host = item_printer
  .work = 10
  .energy = 24
  .inputs = 8
  .outputs = 1
  .tape = {
    [000] = item_elem_g,
    [001] = item_elem_h,
    [002] = item_elem_h,
    [003] = item_elem_g,
    [004] = item_elem_g,
    [005] = item_elem_h,
    [006] = item_elem_h,
    [007] = item_elem_g,
    [008] = item_galvanic,
  }
});

tape_register(item_biosteel, 14, {
  .id = item_biosteel
  .host = item_printer
  .work = 16
  .energy = 64
  .inputs = 11
  .outputs = 3
  .tape = {
    [000] = item_elem_b,
    [001] = item_elem_b,
    [002] = item_elem_h,
    [003] = item_elem_b,
    [004] = item_elem_b,
    [005] = item_elem_m,
    [006] = item_elem_b,
    [007] = item_elem_b,
    [008] = item_elem_h,
    [009] = item_elem_b,
    [010] = item_elem_b,
    [011] = item_elem_o,
    [012] = item_biosteel,
    [013] = item_elem_o,
  }
});

tape_register(item_neurosteel, 14, {
  .id = item_neurosteel
  .host = item_printer
  .work = 16
  .energy = 72
  .inputs = 11
  .outputs = 3
  .tape = {
    [000] = item_elem_d,
    [001] = item_elem_d,
    [002] = item_elem_h,
    [003] = item_elem_d,
    [004] = item_elem_d,
    [005] = item_elem_m,
    [006] = item_elem_d,
    [007] = item_elem_d,
    [008] = item_elem_h,
    [009] = item_elem_d,
    [010] = item_elem_d,
    [011] = item_elem_o,
    [012] = item_neurosteel,
    [013] = item_elem_o,
  }
});

tape_register(item_limb, 7, {
  .id = item_limb
  .host = item_assembly
  .work = 2
  .energy = 4
  .inputs = 6
  .outputs = 1
  .tape = {
    [000] = item_bone,
    [001] = item_bone,
    [002] = item_tendon,
    [003] = item_tendon,
    [004] = item_muscle,
    [005] = item_muscle,
    [006] = item_limb,
  }
});

tape_register(item_spinal, 6, {
  .id = item_spinal
  .host = item_assembly
  .work = 2
  .energy = 4
  .inputs = 5
  .outputs = 1
  .tape = {
    [000] = item_nerve,
    [001] = item_vein,
    [002] = item_nerve,
    [003] = item_vein,
    [004] = item_nerve,
    [005] = item_spinal,
  }
});

tape_register(item_stem, 6, {
  .id = item_stem
  .host = item_assembly
  .work = 2
  .energy = 4
  .inputs = 5
  .outputs = 1
  .tape = {
    [000] = item_neuron,
    [001] = item_neuron,
    [002] = item_vein,
    [003] = item_neuron,
    [004] = item_neuron,
    [005] = item_stem,
  }
});

tape_register(item_lung, 7, {
  .id = item_lung
  .host = item_assembly
  .work = 3
  .energy = 4
  .inputs = 6
  .outputs = 1
  .tape = {
    [000] = item_stem,
    [001] = item_nerve,
    [002] = item_muscle,
    [003] = item_muscle,
    [004] = item_muscle,
    [005] = item_nerve,
    [006] = item_lung,
  }
});

tape_register(item_engram, 8, {
  .id = item_engram
  .host = item_assembly
  .work = 4
  .energy = 6
  .inputs = 7
  .outputs = 1
  .tape = {
    [000] = item_nerve,
    [001] = item_neuron,
    [002] = item_neuron,
    [003] = item_nerve,
    [004] = item_neuron,
    [005] = item_neuron,
    [006] = item_nerve,
    [007] = item_engram,
  }
});

tape_register(item_cortex, 11, {
  .id = item_cortex
  .host = item_assembly
  .work = 8
  .energy = 8
  .inputs = 10
  .outputs = 1
  .tape = {
    [000] = item_stem,
    [001] = item_vein,
    [002] = item_neuron,
    [003] = item_neuron,
    [004] = item_neuron,
    [005] = item_vein,
    [006] = item_neuron,
    [007] = item_neuron,
    [008] = item_neuron,
    [009] = item_vein,
    [010] = item_cortex,
  }
});

tape_register(item_eye, 6, {
  .id = item_eye
  .host = item_assembly
  .work = 2
  .energy = 6
  .inputs = 5
  .outputs = 1
  .tape = {
    [000] = item_retina,
    [001] = item_lens,
    [002] = item_lens,
    [003] = item_lens,
    [004] = item_retina,
    [005] = item_eye,
  }
});

tape_register(item_fusion, 16, {
  .id = item_fusion
  .host = item_assembly
  .work = 16
  .energy = 16
  .inputs = 15
  .outputs = 1
  .tape = {
    [000] = item_torus,
    [001] = item_rod,
    [002] = item_rod,
    [003] = item_rod,
    [004] = item_rod,
    [005] = item_torus,
    [006] = item_stem,
    [007] = item_cortex,
    [008] = item_stem,
    [009] = item_torus,
    [010] = item_rod,
    [011] = item_rod,
    [012] = item_rod,
    [013] = item_rod,
    [014] = item_torus,
    [015] = item_fusion,
  }
});

tape_register(item_photovoltaic, 9, {
  .id = item_photovoltaic
  .host = item_assembly
  .work = 8
  .energy = 16
  .inputs = 8
  .outputs = 1
  .tape = {
    [000] = item_semiconductor,
    [001] = item_semiconductor,
    [002] = item_nerve,
    [003] = item_bone,
    [004] = item_bone,
    [005] = item_nerve,
    [006] = item_semiconductor,
    [007] = item_semiconductor,
    [008] = item_photovoltaic,
  }
});

tape_register(item_field, 9, {
  .id = item_field
  .host = item_assembly
  .work = 8
  .energy = 16
  .inputs = 8
  .outputs = 1
  .tape = {
    [000] = item_magnet,
    [001] = item_magnet,
    [002] = item_magnet,
    [003] = item_nerve,
    [004] = item_nerve,
    [005] = item_magnet,
    [006] = item_magnet,
    [007] = item_magnet,
    [008] = item_field,
  }
});

tape_register(item_antenna, 12, {
  .id = item_antenna
  .host = item_assembly
  .work = 12
  .energy = 24
  .inputs = 11
  .outputs = 1
  .tape = {
    [000] = item_bone,
    [001] = item_bone,
    [002] = item_nerve,
    [003] = item_conductor,
    [004] = item_conductor,
    [005] = item_nerve,
    [006] = item_conductor,
    [007] = item_conductor,
    [008] = item_nerve,
    [009] = item_bone,
    [010] = item_bone,
    [011] = item_antenna,
  }
});

tape_register(item_accelerator, 9, {
  .id = item_accelerator
  .host = item_assembly
  .work = 16
  .energy = 64
  .inputs = 8
  .outputs = 1
  .tape = {
    [000] = item_conductor,
    [001] = item_conductor,
    [002] = item_nerve,
    [003] = item_field,
    [004] = item_field,
    [005] = item_nerve,
    [006] = item_conductor,
    [007] = item_conductor,
    [008] = item_accelerator,
  }
});

tape_register(item_heat_exchange, 6, {
  .id = item_heat_exchange
  .host = item_assembly
  .work = 16
  .energy = 72
  .inputs = 5
  .outputs = 1
  .tape = {
    [000] = item_biosteel,
    [001] = item_conductor,
    [002] = item_conductor,
    [003] = item_conductor,
    [004] = item_biosteel,
    [005] = item_heat_exchange,
  }
});

tape_register(item_furnace, 10, {
  .id = item_furnace
  .host = item_assembly
  .work = 20
  .energy = 76
  .inputs = 7
  .outputs = 3
  .tape = {
    [000] = item_biosteel,
    [001] = item_heat_exchange,
    [002] = item_heat_exchange,
    [003] = item_elem_m,
    [004] = item_heat_exchange,
    [005] = item_heat_exchange,
    [006] = item_biosteel,
    [007] = item_elem_o,
    [008] = item_furnace,
    [009] = item_elem_o,
  }
});

tape_register(item_freezer, 8, {
  .id = item_freezer
  .host = item_assembly
  .work = 20
  .energy = 76
  .inputs = 7
  .outputs = 1
  .tape = {
    [000] = item_heat_exchange,
    [001] = item_heat_exchange,
    [002] = item_neurosteel,
    [003] = item_elem_m,
    [004] = item_neurosteel,
    [005] = item_heat_exchange,
    [006] = item_heat_exchange,
    [007] = item_freezer,
  }
});

tape_register(item_m_reactor, 16, {
  .id = item_m_reactor
  .host = item_assembly
  .work = 32
  .energy = 128
  .inputs = 13
  .outputs = 3
  .tape = {
    [000] = item_biosteel,
    [001] = item_heat_exchange,
    [002] = item_neurosteel,
    [003] = item_elem_m,
    [004] = item_elem_m,
    [005] = item_neurosteel,
    [006] = item_elem_m,
    [007] = item_neurosteel,
    [008] = item_elem_m,
    [009] = item_elem_m,
    [010] = item_neurosteel,
    [011] = item_heat_exchange,
    [012] = item_biosteel,
    [013] = item_elem_o,
    [014] = item_m_reactor,
    [015] = item_elem_o,
  }
});

tape_register(item_m_condenser, 14, {
  .id = item_m_condenser
  .host = item_assembly
  .work = 32
  .energy = 128
  .inputs = 11
  .outputs = 3
  .tape = {
    [000] = item_biosteel,
    [001] = item_field,
    [002] = item_elem_m,
    [003] = item_elem_m,
    [004] = item_field,
    [005] = item_elem_m,
    [006] = item_field,
    [007] = item_elem_m,
    [008] = item_elem_m,
    [009] = item_field,
    [010] = item_biosteel,
    [011] = item_elem_o,
    [012] = item_m_condenser,
    [013] = item_elem_o,
  }
});

tape_register(item_m_release, 12, {
  .id = item_m_release
  .host = item_assembly
  .work = 32
  .energy = 144
  .inputs = 9
  .outputs = 3
  .tape = {
    [000] = item_biosteel,
    [001] = item_ferrofluid,
    [002] = item_ferrofluid,
    [003] = item_ferrofluid,
    [004] = item_elem_m,
    [005] = item_ferrofluid,
    [006] = item_ferrofluid,
    [007] = item_ferrofluid,
    [008] = item_biosteel,
    [009] = item_elem_o,
    [010] = item_m_release,
    [011] = item_elem_o,
  }
});

tape_register(item_m_lung, 10, {
  .id = item_m_lung
  .host = item_assembly
  .work = 40
  .energy = 256
  .inputs = 9
  .outputs = 1
  .tape = {
    [000] = item_biosteel,
    [001] = item_m_reactor,
    [002] = item_neurosteel,
    [003] = item_m_condenser,
    [004] = item_m_condenser,
    [005] = item_neurosteel,
    [006] = item_m_release,
    [007] = item_m_release,
    [008] = item_m_release,
    [009] = item_m_lung,
  }
});

tape_register(item_worker, 8, {
  .id = item_worker
  .host = item_assembly
  .work = 6
  .energy = 16
  .inputs = 7
  .outputs = 1
  .tape = {
    [000] = item_lung,
    [001] = item_lung,
    [002] = item_stem,
    [003] = item_nodule,
    [004] = item_stem,
    [005] = item_limb,
    [006] = item_limb,
    [007] = item_worker,
  }
});

tape_register(item_solar, 6, {
  .id = item_solar
  .host = item_assembly
  .work = 6
  .energy = 16
  .inputs = 5
  .outputs = 1
  .tape = {
    [000] = item_photovoltaic,
    [001] = item_nerve,
    [002] = item_nodule,
    [003] = item_nerve,
    [004] = item_photovoltaic,
    [005] = item_solar,
  }
});

tape_register(item_pill, 9, {
  .id = item_pill
  .host = item_assembly
  .work = 8
  .energy = 24
  .inputs = 8
  .outputs = 1
  .tape = {
    [000] = item_vein,
    [001] = item_ferrofluid,
    [002] = item_vein,
    [003] = item_bone,
    [004] = item_bone,
    [005] = item_vein,
    [006] = item_ferrofluid,
    [007] = item_vein,
    [008] = item_pill,
  }
});

tape_register(item_battery, 8, {
  .id = item_battery
  .host = item_assembly
  .work = 4
  .energy = 8
  .inputs = 7
  .outputs = 1
  .tape = {
    [000] = item_bone,
    [001] = item_nerve,
    [002] = item_galvanic,
    [003] = item_galvanic,
    [004] = item_galvanic,
    [005] = item_nerve,
    [006] = item_bone,
    [007] = item_battery,
  }
});

tape_register(item_elem_m, 13, {
  .id = item_elem_m
  .host = item_collider
  .work = 16
  .energy = 128
  .inputs = 11
  .outputs = 2
  .tape = {
    [000] = item_elem_a,
    [001] = item_elem_a,
    [002] = item_elem_a,
    [003] = item_elem_d,
    [004] = item_elem_d,
    [005] = item_elem_g,
    [006] = item_elem_d,
    [007] = item_elem_d,
    [008] = item_elem_a,
    [009] = item_elem_a,
    [010] = item_elem_a,
    [011] = item_elem_m,
    [012] = item_elem_o,
  }
});

tape_register(item_elem_n, 23, {
  .id = item_elem_n
  .host = item_collider
  .work = 24
  .energy = 256
  .inputs = 21
  .outputs = 2
  .tape = {
    [000] = item_elem_b,
    [001] = item_elem_b,
    [002] = item_elem_b,
    [003] = item_elem_b,
    [004] = item_elem_b,
    [005] = item_elem_d,
    [006] = item_elem_d,
    [007] = item_elem_d,
    [008] = item_elem_d,
    [009] = item_elem_e,
    [010] = item_elem_e,
    [011] = item_elem_e,
    [012] = item_elem_d,
    [013] = item_elem_d,
    [014] = item_elem_d,
    [015] = item_elem_d,
    [016] = item_elem_b,
    [017] = item_elem_b,
    [018] = item_elem_b,
    [019] = item_elem_b,
    [020] = item_elem_b,
    [021] = item_elem_n,
    [022] = item_elem_o,
  }
});

tape_register(item_elem_o, 0, {
  .id = item_elem_o
  .host = item_dummy
  .work = 0
  .energy = 0
  .inputs = 0
  .outputs = 0
  .tape = {
  }
});

tape_register(item_memory, 8, {
  .id = item_memory
  .host = item_assembly
  .work = 8
  .energy = 16
  .inputs = 7
  .outputs = 1
  .tape = {
    [000] = item_engram,
    [001] = item_engram,
    [002] = item_engram,
    [003] = item_nodule,
    [004] = item_engram,
    [005] = item_engram,
    [006] = item_engram,
    [007] = item_memory,
  }
});

tape_register(item_brain, 8, {
  .id = item_brain
  .host = item_assembly
  .work = 10
  .energy = 18
  .inputs = 7
  .outputs = 1
  .tape = {
    [000] = item_memory,
    [001] = item_cortex,
    [002] = item_cortex,
    [003] = item_nodule,
    [004] = item_cortex,
    [005] = item_cortex,
    [006] = item_memory,
    [007] = item_brain,
  }
});

tape_register(item_prober, 6, {
  .id = item_prober
  .host = item_assembly
  .work = 4
  .energy = 18
  .inputs = 5
  .outputs = 1
  .tape = {
    [000] = item_lung,
    [001] = item_nodule,
    [002] = item_eye,
    [003] = item_eye,
    [004] = item_eye,
    [005] = item_prober,
  }
});

tape_register(item_scanner, 9, {
  .id = item_scanner
  .host = item_assembly
  .work = 6
  .energy = 18
  .inputs = 8
  .outputs = 1
  .tape = {
    [000] = item_lung,
    [001] = item_engram,
    [002] = item_nodule,
    [003] = item_eye,
    [004] = item_eye,
    [005] = item_eye,
    [006] = item_eye,
    [007] = item_eye,
    [008] = item_scanner,
  }
});

tape_register(item_legion, 22, {
  .id = item_legion
  .host = item_assembly
  .work = 24
  .energy = 32
  .inputs = 21
  .outputs = 1
  .tape = {
    [000] = item_lung,
    [001] = item_lung,
    [002] = item_lung,
    [003] = item_nodule,
    [004] = item_worker,
    [005] = item_worker,
    [006] = item_fusion,
    [007] = item_extract,
    [008] = item_extract,
    [009] = item_printer,
    [010] = item_printer,
    [011] = item_assembly,
    [012] = item_assembly,
    [013] = item_deploy,
    [014] = item_deploy,
    [015] = item_memory,
    [016] = item_brain,
    [017] = item_prober,
    [018] = item_nodule,
    [019] = item_cortex,
    [020] = item_cortex,
    [021] = item_legion,
  }
});

tape_register(item_transmit, 8, {
  .id = item_transmit
  .host = item_assembly
  .work = 16
  .energy = 20
  .inputs = 7
  .outputs = 1
  .tape = {
    [000] = item_eye,
    [001] = item_eye,
    [002] = item_eye,
    [003] = item_nodule,
    [004] = item_antenna,
    [005] = item_antenna,
    [006] = item_antenna,
    [007] = item_transmit,
  }
});

tape_register(item_receive, 8, {
  .id = item_receive
  .host = item_assembly
  .work = 16
  .energy = 20
  .inputs = 7
  .outputs = 1
  .tape = {
    [000] = item_antenna,
    [001] = item_antenna,
    [002] = item_antenna,
    [003] = item_nodule,
    [004] = item_engram,
    [005] = item_engram,
    [006] = item_engram,
    [007] = item_receive,
  }
});

tape_register(item_nomad, 14, {
  .id = item_nomad
  .host = item_assembly
  .work = 42
  .energy = 256
  .inputs = 13
  .outputs = 1
  .tape = {
    [000] = item_biosteel,
    [001] = item_pill,
    [002] = item_brain,
    [003] = item_memory,
    [004] = item_pill,
    [005] = item_packer,
    [006] = item_storage,
    [007] = item_storage,
    [008] = item_storage,
    [009] = item_pill,
    [010] = item_biosteel,
    [011] = item_m_lung,
    [012] = item_m_lung,
    [013] = item_nomad,
  }
});

tape_register(item_deploy, 4, {
  .id = item_deploy
  .host = item_assembly
  .work = 4
  .energy = 8
  .inputs = 3
  .outputs = 1
  .tape = {
    [000] = item_spinal,
    [001] = item_nodule,
    [002] = item_spinal,
    [003] = item_deploy,
  }
});

tape_register(item_extract, 6, {
  .id = item_extract
  .host = item_assembly
  .work = 4
  .energy = 8
  .inputs = 5
  .outputs = 1
  .tape = {
    [000] = item_muscle,
    [001] = item_muscle,
    [002] = item_nodule,
    [003] = item_muscle,
    [004] = item_muscle,
    [005] = item_extract,
  }
});

tape_register(item_printer, 6, {
  .id = item_printer
  .host = item_assembly
  .work = 4
  .energy = 16
  .inputs = 5
  .outputs = 1
  .tape = {
    [000] = item_vein,
    [001] = item_vein,
    [002] = item_nodule,
    [003] = item_vein,
    [004] = item_vein,
    [005] = item_printer,
  }
});

tape_register(item_assembly, 8, {
  .id = item_assembly
  .host = item_assembly
  .work = 4
  .energy = 16
  .inputs = 7
  .outputs = 1
  .tape = {
    [000] = item_limb,
    [001] = item_limb,
    [002] = item_vein,
    [003] = item_nodule,
    [004] = item_vein,
    [005] = item_limb,
    [006] = item_limb,
    [007] = item_assembly,
  }
});

tape_register(item_lab, 9, {
  .id = item_lab
  .host = item_assembly
  .work = 16
  .energy = 32
  .inputs = 8
  .outputs = 1
  .tape = {
    [000] = item_limb,
    [001] = item_limb,
    [002] = item_vein,
    [003] = item_nodule,
    [004] = item_nodule,
    [005] = item_vein,
    [006] = item_cortex,
    [007] = item_cortex,
    [008] = item_lab,
  }
});

tape_register(item_storage, 8, {
  .id = item_storage
  .host = item_assembly
  .work = 8
  .energy = 16
  .inputs = 7
  .outputs = 1
  .tape = {
    [000] = item_bone,
    [001] = item_bone,
    [002] = item_vein,
    [003] = item_vein,
    [004] = item_vein,
    [005] = item_bone,
    [006] = item_bone,
    [007] = item_storage,
  }
});

tape_register(item_port, 12, {
  .id = item_port
  .host = item_assembly
  .work = 24
  .energy = 32
  .inputs = 11
  .outputs = 1
  .tape = {
    [000] = item_field,
    [001] = item_field,
    [002] = item_field,
    [003] = item_field,
    [004] = item_field,
    [005] = item_nodule,
    [006] = item_limb,
    [007] = item_limb,
    [008] = item_limb,
    [009] = item_nodule,
    [010] = item_storage,
    [011] = item_port,
  }
});

tape_register(item_condenser, 7, {
  .id = item_condenser
  .host = item_assembly
  .work = 10
  .energy = 24
  .inputs = 6
  .outputs = 1
  .tape = {
    [000] = item_lung,
    [001] = item_lung,
    [002] = item_nodule,
    [003] = item_lung,
    [004] = item_lung,
    [005] = item_lung,
    [006] = item_condenser,
  }
});

tape_register(item_collider, 10, {
  .id = item_collider
  .host = item_assembly
  .work = 16
  .energy = 128
  .inputs = 9
  .outputs = 1
  .tape = {
    [000] = item_brain,
    [001] = item_nodule,
    [002] = item_accelerator,
    [003] = item_accelerator,
    [004] = item_nodule,
    [005] = item_accelerator,
    [006] = item_accelerator,
    [007] = item_nodule,
    [008] = item_storage,
    [009] = item_collider,
  }
});

tape_register(item_burner, 11, {
  .id = item_burner
  .host = item_assembly
  .work = 20
  .energy = 112
  .inputs = 10
  .outputs = 1
  .tape = {
    [000] = item_biosteel,
    [001] = item_furnace,
    [002] = item_furnace,
    [003] = item_furnace,
    [004] = item_heat_exchange,
    [005] = item_conductor,
    [006] = item_battery,
    [007] = item_battery,
    [008] = item_battery,
    [009] = item_biosteel,
    [010] = item_burner,
  }
});

tape_register(item_packer, 12, {
  .id = item_packer
  .host = item_assembly
  .work = 20
  .energy = 112
  .inputs = 11
  .outputs = 1
  .tape = {
    [000] = item_biosteel,
    [001] = item_neurosteel,
    [002] = item_freezer,
    [003] = item_freezer,
    [004] = item_freezer,
    [005] = item_worker,
    [006] = item_storage,
    [007] = item_storage,
    [008] = item_storage,
    [009] = item_neurosteel,
    [010] = item_biosteel,
    [011] = item_packer,
  }
});

tape_register(item_elem_a, 1, {
  .id = item_elem_a
  .host = item_extract
  .work = 1
  .energy = 1
  .inputs = 0
  .outputs = 1
  .tape = {
    [000] = item_elem_a,
  }
});

tape_register(item_elem_b, 1, {
  .id = item_elem_b
  .host = item_extract
  .work = 2
  .energy = 1
  .inputs = 0
  .outputs = 1
  .tape = {
    [000] = item_elem_b,
  }
});

tape_register(item_elem_c, 1, {
  .id = item_elem_c
  .host = item_extract
  .work = 2
  .energy = 2
  .inputs = 0
  .outputs = 1
  .tape = {
    [000] = item_elem_c,
  }
});

tape_register(item_elem_d, 1, {
  .id = item_elem_d
  .host = item_extract
  .work = 4
  .energy = 4
  .inputs = 0
  .outputs = 1
  .tape = {
    [000] = item_elem_d,
  }
});

tape_register(item_elem_g, 1, {
  .id = item_elem_g
  .host = item_condenser
  .work = 4
  .energy = 8
  .inputs = 0
  .outputs = 1
  .tape = {
    [000] = item_elem_g,
  }
});

tape_register(item_elem_h, 1, {
  .id = item_elem_h
  .host = item_condenser
  .work = 8
  .energy = 12
  .inputs = 0
  .outputs = 1
  .tape = {
    [000] = item_elem_h,
  }
});

tape_register(item_elem_e, 1, {
  .id = item_elem_e
  .host = item_extract
  .work = 8
  .energy = 32
  .inputs = 0
  .outputs = 1
  .tape = {
    [000] = item_elem_e,
  }
});

tape_register(item_elem_f, 1, {
  .id = item_elem_f
  .host = item_condenser
  .work = 16
  .energy = 64
  .inputs = 0
  .outputs = 1
  .tape = {
    [000] = item_elem_i,
  }
});
