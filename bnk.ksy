meta:
  id: bnk
  title: Wwise BNK Format
  file-extension: bnk
  endian: le
seq:
  - id: bank_header
    type: bank_header
  - id: data_index
    type: data_index
  - id: data
    type: data
types:
  bank_header:
    doc: The BKHD section (Bank Header) contains the version number and the SoundBank id.
    seq:
      - id: magic
        contents: 'BKHD'
      - id: length
        type: u4
      - id: version
        type: u4
      - id: id
        type: u4
      - id: data
        size: length - (2 * 4)
  data_index:
    doc: The DIDX (Data Index) section contains the references to the .wem files embedded in the SoundBank.
    seq:
      - id: magic
        contents: 'DIDX'
      - id: length
        type: u4
      - id: data
        type: indices
        size: length
  indices:
    seq:
      - id: indices
        type: index
        repeat: eos
  index:
    seq:
      - id: id
        type: u4
      - id: offset
        type: u4
      - id: length
        type: u4
  data:
    seq:
      - id: magic
        contents: 'DATA'
      - id: length
        type: u4
      - id: data
        size: length
        type: audio_sections
  audio_sections:
    seq:
      - id: audio_sections
        type: audio_section(_index)
        repeat: expr
        repeat-expr: _parent._parent.data_index.data.indices.size
  audio_section:
    params:
      - id: index
        type: u4
    instances:
      data:
        pos: _parent._parent._parent.data_index.data.indices[index].offset
        size: _parent._parent._parent.data_index.data.indices[index].length