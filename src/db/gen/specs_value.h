/* This file was generated by ./legion --db */
#pragma once

enum
{
  im_muscle_lab_bits = 0x8,
  im_muscle_lab_work = 0x8,
  im_muscle_lab_energy = 0x8,

  im_nodule_lab_bits = 0x8,
  im_nodule_lab_work = 0x8,
  im_nodule_lab_energy = 0x8,

  im_vein_lab_bits = 0x8,
  im_vein_lab_work = 0x8,
  im_vein_lab_energy = 0x8,

  im_bone_lab_bits = 0x8,
  im_bone_lab_work = 0x8,
  im_bone_lab_energy = 0x8,

  im_tendon_lab_bits = 0x8,
  im_tendon_lab_work = 0x8,
  im_tendon_lab_energy = 0x8,

  im_rod_lab_bits = 0x8,
  im_rod_lab_work = 0x8,
  im_rod_lab_energy = 0x8,

  im_torus_lab_bits = 0x8,
  im_torus_lab_work = 0x8,
  im_torus_lab_energy = 0x8,

  im_lens_lab_bits = 0x8,
  im_lens_lab_work = 0x8,
  im_lens_lab_energy = 0x8,

  im_nerve_lab_bits = 0x8,
  im_nerve_lab_work = 0x8,
  im_nerve_lab_energy = 0x8,

  im_neuron_lab_bits = 0x8,
  im_neuron_lab_work = 0x8,
  im_neuron_lab_energy = 0x8,

  im_retina_lab_bits = 0x8,
  im_retina_lab_work = 0x8,
  im_retina_lab_energy = 0x8,

  im_magnet_lab_bits = 0x8,
  im_magnet_lab_work = 0x8,
  im_magnet_lab_energy = 0x8,

  im_ferrofluid_lab_bits = 0x8,
  im_ferrofluid_lab_work = 0x8,
  im_ferrofluid_lab_energy = 0x8,

  im_semiconductor_lab_bits = 0x8,
  im_semiconductor_lab_work = 0x8,
  im_semiconductor_lab_energy = 0x8,

  im_conductor_lab_bits = 0x8,
  im_conductor_lab_work = 0x8,
  im_conductor_lab_energy = 0x8,

  im_galvanic_lab_bits = 0x8,
  im_galvanic_lab_work = 0x8,
  im_galvanic_lab_energy = 0x8,

  im_biosteel_lab_bits = 0x8,
  im_biosteel_lab_work = 0x8,
  im_biosteel_lab_energy = 0x8,

  im_neurosteel_lab_bits = 0x8,
  im_neurosteel_lab_work = 0x8,
  im_neurosteel_lab_energy = 0x8,

  im_limb_lab_bits = 0x8,
  im_limb_lab_work = 0x8,
  im_limb_lab_energy = 0x8,

  im_spinal_lab_bits = 0x8,
  im_spinal_lab_work = 0x8,
  im_spinal_lab_energy = 0x8,

  im_stem_lab_bits = 0x8,
  im_stem_lab_work = 0x8,
  im_stem_lab_energy = 0x8,

  im_lung_lab_bits = 0x8,
  im_lung_lab_work = 0x8,
  im_lung_lab_energy = 0x8,

  im_engram_lab_bits = 0x8,
  im_engram_lab_work = 0x8,
  im_engram_lab_energy = 0x8,

  im_cortex_lab_bits = 0x8,
  im_cortex_lab_work = 0x8,
  im_cortex_lab_energy = 0x8,

  im_eye_lab_bits = 0x8,
  im_eye_lab_work = 0x8,
  im_eye_lab_energy = 0x8,

  im_photovoltaic_lab_bits = 0x8,
  im_photovoltaic_lab_work = 0x8,
  im_photovoltaic_lab_energy = 0x8,

  im_field_lab_bits = 0x8,
  im_field_lab_work = 0x8,
  im_field_lab_energy = 0x8,

  im_antenna_lab_bits = 0x8,
  im_antenna_lab_work = 0x8,
  im_antenna_lab_energy = 0x8,

  im_accelerator_lab_bits = 0x8,
  im_accelerator_lab_work = 0x8,
  im_accelerator_lab_energy = 0x8,

  im_heat_exchange_lab_bits = 0x8,
  im_heat_exchange_lab_work = 0x8,
  im_heat_exchange_lab_energy = 0x8,

  im_furnace_lab_bits = 0x8,
  im_furnace_lab_work = 0x8,
  im_furnace_lab_energy = 0x8,

  im_freezer_lab_bits = 0x8,
  im_freezer_lab_work = 0x8,
  im_freezer_lab_energy = 0x8,

  im_m_reactor_lab_bits = 0x8,
  im_m_reactor_lab_work = 0x8,
  im_m_reactor_lab_energy = 0x8,

  im_m_condenser_lab_bits = 0x8,
  im_m_condenser_lab_work = 0x8,
  im_m_condenser_lab_energy = 0x8,

  im_m_release_lab_bits = 0x8,
  im_m_release_lab_work = 0x8,
  im_m_release_lab_energy = 0x8,

  im_m_lung_lab_bits = 0x8,
  im_m_lung_lab_work = 0x8,
  im_m_lung_lab_energy = 0x8,

  im_worker_lab_bits = 0x8,
  im_worker_lab_work = 0x10,
  im_worker_lab_energy = 0x8,

  im_solar_lab_bits = 0x10,
  im_solar_lab_work = 0x20,
  im_solar_lab_energy = 0x10,
  im_solar_energy_div = 0x400,

  im_pill_lab_bits = 0x10,
  im_pill_lab_work = 0x20,
  im_pill_lab_energy = 0x20,

  im_battery_lab_bits = 0x10,
  im_battery_lab_work = 0x18,
  im_battery_lab_energy = 0x20,

  im_elem_m_lab_bits = 0x10,
  im_elem_m_lab_work = 0x80,
  im_elem_m_lab_energy = 0x40,

  im_elem_n_lab_bits = 0x10,
  im_elem_n_lab_work = 0xc0,
  im_elem_n_lab_energy = 0x60,

  im_elem_o_lab_bits = 0x80,
  im_elem_o_lab_work = 0x8,
  im_elem_o_lab_energy = 0x10,

  im_memory_lab_bits = 0x8,
  im_memory_lab_work = 0x8,
  im_memory_lab_energy = 0x8,

  im_brain_lab_bits = 0x8,
  im_brain_lab_work = 0x8,
  im_brain_lab_energy = 0x8,

  im_prober_lab_bits = 0x8,
  im_prober_lab_work = 0x8,
  im_prober_lab_energy = 0x8,

  im_scanner_lab_bits = 0x8,
  im_scanner_lab_work = 0x8,
  im_scanner_lab_energy = 0x8,

  im_legion_lab_bits = 0x8,
  im_legion_lab_work = 0x8,
  im_legion_lab_energy = 0x8,

  im_transmit_lab_bits = 0x8,
  im_transmit_lab_work = 0x8,
  im_transmit_lab_energy = 0x8,

  im_receive_lab_bits = 0x8,
  im_receive_lab_work = 0x8,
  im_receive_lab_energy = 0x8,

  im_deploy_lab_bits = 0x8,
  im_deploy_lab_work = 0x8,
  im_deploy_lab_energy = 0x8,

  im_extract_lab_bits = 0x8,
  im_extract_lab_work = 0x8,
  im_extract_lab_energy = 0x8,

  im_printer_lab_bits = 0x8,
  im_printer_lab_work = 0x8,
  im_printer_lab_energy = 0x8,

  im_assembly_lab_bits = 0x8,
  im_assembly_lab_work = 0x8,
  im_assembly_lab_energy = 0x8,

  im_fusion_lab_bits = 0x8,
  im_fusion_lab_work = 0x8,
  im_fusion_lab_energy = 0x8,

  im_lab_lab_bits = 0x8,
  im_lab_lab_work = 0x8,
  im_lab_lab_energy = 0x8,

  im_storage_lab_bits = 0x8,
  im_storage_lab_work = 0x8,
  im_storage_lab_energy = 0x8,

  im_port_lab_bits = 0x8,
  im_port_lab_work = 0x8,
  im_port_lab_energy = 0x8,

  im_condenser_lab_bits = 0x8,
  im_condenser_lab_work = 0x8,
  im_condenser_lab_energy = 0x8,

  im_collider_lab_bits = 0x8,
  im_collider_lab_work = 0x8,
  im_collider_lab_energy = 0x8,

  im_burner_lab_bits = 0x8,
  im_burner_lab_work = 0x8,
  im_burner_lab_energy = 0x8,

  im_packer_lab_bits = 0x8,
  im_packer_lab_work = 0x8,
  im_packer_lab_energy = 0x8,

  im_nomad_lab_bits = 0x8,
  im_nomad_lab_work = 0x8,
  im_nomad_lab_energy = 0x8,

  im_elem_a_lab_bits = 0x8,
  im_elem_a_lab_work = 0x1,
  im_elem_a_lab_energy = 0x1,

  im_elem_b_lab_bits = 0x8,
  im_elem_b_lab_work = 0x2,
  im_elem_b_lab_energy = 0x2,

  im_elem_c_lab_bits = 0x8,
  im_elem_c_lab_work = 0x4,
  im_elem_c_lab_energy = 0x4,

  im_elem_d_lab_bits = 0x8,
  im_elem_d_lab_work = 0x4,
  im_elem_d_lab_energy = 0x4,

  im_elem_g_lab_bits = 0x10,
  im_elem_g_lab_work = 0x10,
  im_elem_g_lab_energy = 0x8,

  im_elem_h_lab_bits = 0x10,
  im_elem_h_lab_work = 0x20,
  im_elem_h_lab_energy = 0x10,

  im_elem_e_lab_bits = 0x20,
  im_elem_e_lab_work = 0x40,
  im_elem_e_lab_energy = 0x20,

  im_elem_f_lab_bits = 0x20,
  im_elem_f_lab_work = 0x40,
  im_elem_f_lab_energy = 0x30,
};