api_header_deps

foreach header : api_headers
  api_header_deps += configure_file(
    input : header[1],
    output : header[0],
    configuration : configuration_data())
endforeach
