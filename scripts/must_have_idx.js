function must_have_idx(obj, index) {
  if (!obj.hasOwnProperty(index)) {
    throw new Error('Missing index \'' + index + '\'');
  }
  return obj[index];
}

module.exports = must_have_idx;
