% dfilter.butter  Butterworth filter returned as a dfilter.TF object.
%
%   f = dfilter.butter(n, wn)
%   f = dfilter.butter(n, wn, ftype)
%
% Arguments are passed directly to the built-in butter() function.
% See `help butter` for details.
function f = butter(varargin)
    [b, a] = builtin('butter', varargin{:});
    f = dfilter.TF(b, a);
end
