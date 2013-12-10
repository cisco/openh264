{
    'rules': [
    {
        'rule_name':'assembler',
        'extension':'asm',
        'inputs':[],
        'outputs':[
            '<(PRODUCT_DIR)/<(asm_path)/<(RULE_INPUT_ROOT).o',
            ],
        'action':[
            'nasm', '-f', '<@(asm_format)', '<@(asm_prefix)', '-DNO_DYNAMIC_VP', '-DNOPREFIX', '-Iprocessing/src/asm/', '<(RULE_INPUT_PATH)', '-o', '<(PRODUCT_DIR)/<(asm_path)/<(RULE_INPUT_ROOT).o'
            ],
        'message':
            'Assembling <(RULE_INPUT_PATH) to <(RULE_INPUT_ROOT)',
        'process_outputs_as_sources':1
        }
    ]
}
