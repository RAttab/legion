;; -----------------------------------------------------------------------------
;; passives
;; -----------------------------------------------------------------------------

(item_assembly_1

 (item_servo
  (in item_gear
      item_frame
      item_gear
      item_frame
      item_gear
      item_frame
      item_gear)
  (out item_servo))

 (item_thruster
  (in item_frame
      item_servo
      item_fuel
      item_fuel
      item_fuel
      item_fuel
      item_fuel
      item_servo
      item_frame)
  (out item_thruster))

 (item_propulsion
  (in item_frame
      item_servo
      item_thruster
      item_bonding
      item_bonding
      item_bonding
      item_bonding
      item_bonding
      item_thruster
      item_servo
      item_frame)
  (out item_propulsion))

 (item_plate
  (in item_frame
      item_bonding
      item_frame
      item_bonding
      item_frame
      item_bonding
      item_frame
      item_bonding
      item_frame)
  (out item_plate))

 (item_shielding
  (in item_circuit
      item_bonding
      item_circuit
      item_bonding
      item_circuit
      item_bonding
      item_circuit
      item_bonding
      item_circuit)
  (out item_shielding))

 (item_hull_1
  (in item_plate
      item_bonding
      item_shielding
      item_bonding
      item_propulsion
      item_bonding
      item_core
      item_bonding
      item_propulsion
      item_bonding
      item_shielding
      item_bonding
      item_plate)
  (out item_hull_1))

 (item_core
  (in item_circuit
      item_neural
      item_circuit
      item_neural
      item_circuit
      item_neural
      item_circuit
      item_neural
      item_circuit)
  (out item_core))

 (item_matrix
  (in item_core
      item_bonding
      item_core
      item_bonding
      item_core
      item_bonding
      item_core
      item_bonding
      item_core)
  (out item_matrix))

 (item_databank
  (in item_circuit
      item_frame
      item_circuit
      item_frame
      item_circuit
      item_frame
      item_circuit
      item_frame
      item_circuit)
  (out item_databank)))


;; -----------------------------------------------------------------------------
;; logistics
;; -----------------------------------------------------------------------------

(item_assembly_1

 (item_worker
  (in item_thruster
      item_core
      item_frame
      item_core
      item_thruster
      item_core
      item_frame
      item_core
      item_thruster)
  (out item_worker)))


;; -----------------------------------------------------------------------------
;; active
;; -----------------------------------------------------------------------------

(item_assembly_1

 (item_deploy
  (in item_circuit
      item_frame
      item_circuit
      item_frame
      item_circuit
      item_frame
      item_circuit
      item_frame
      item_circuit)
  (out item_deploy))

 (item_extract_1
  (in item_circuit
      item_frame
      item_circuit
      item_frame
      item_circuit
      item_frame
      item_circuit
      item_frame
      item_circuit)
  (out item_extract_1))

 (item_printer_1
  (in item_gear
      item_circuit
      item_gear
      item_circuit
      item_gear
      item_circuit
      item_gear
      item_circuit
      item_gear)
  (out item_printer_1))

 (item_assembly_1
  (in item_servo
      item_neural
      item_frame
      item_neural
      item_servo
      item_neural
      item_frame
      item_neural
      item_servo)
  (out item_assembly_1))

 (item_storage
  (in item_servo
      item_frame
      item_servo
      item_frame
      item_servo
      item_frame
      item_servo
      item_frame
      item_servo)
  (out item_storage))

 (item_scanner_1
  (in item_plate
      item_servo
      item_servo
      item_databank
      item_core
      item_databank
      item_servo
      item_servo
      item_plate)
  (out item_scanner_1))

 (item_db_1
  (in item_databank
      item_core
      item_frame
      item_core
      item_databank
      item_core
      item_frame
      item_core
      item_databank)
  (out item_db_1))

 (item_brain_1
  (in item_databank
      item_matrix
      item_frame
      item_matrix
      item_databank
      item_matrix
      item_frame
      item_matrix
      item_databank)
  (out item_brain_1))

 (item_legion_1
  (in item_hull_1
      item_hull_1
      item_worker
      item_worker
      item_deploy
      item_extract_1
      item_extract_1
      item_printer_1
      item_printer_1
      item_assembly_1
      item_assembly_1
      item_scanner_1
      item_db_1
      item_brain_1
      item_hull_1
      item_hull_1)
  (out item_legion_1)))
