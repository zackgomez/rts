function invariant_violation(message) {
  throw new Error('invariant_violation: ' + message);
}

function invariant(condition, message) {
  if (!condition) {
    invariant_violation(message);
  }
}

exports.invariant = invariant;
exports.invariant_violation = invariant_violation;
