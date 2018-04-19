api_header_deps

foreach header : api_headers
  api_header_deps += custom_target(header[0],
    build_by_default: true,
    input : header[1],
    output : header[0],
    command : [filecopier, '@INPUT@', '@OUTPUT@'])
endforeach
