# hash

Hash string contents for easy checksums.

### hash.new ([strings...])

Creates a new hash, combining every string in **strings** if specified.
Returns a new `Hex.hash`.

### hash.combine (hash[, strings...])

Combines every string in **strings** to the already existing **hash**. Returning the combination.
Used as a metamethod for the concatenation operator of any `Hex.hash`.

### hash.tostring (hash)

Returns a string representing **hash** hexadecimal content, suitable for printing.
Used as a metamethod for the string cast operator of any `Hex.hash`.

