(io !io_prog (id !item_extract_1 1) !item_elem_a)
(io !io_prog (id !item_extract_1 2) !item_elem_f)

(io !io_prog (id !item_printer_1 1) !item_frame)
(io !io_prog (id !item_printer_1 2) !item_circuit)

(io !io_prog (id !item_assembly_1 1) !item_extract_1)
(io !io_item (id !item_deploy 1) !item_extract_1)

(while 1 (yield))
