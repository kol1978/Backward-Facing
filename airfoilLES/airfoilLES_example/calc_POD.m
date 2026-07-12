
% This is a very basic/minimalistic POD code, intended only for
% demonstration

function [phi,lambda,a] = calc_POD(q,Weights,numout)


% Find data dimensions
siz = size(q);
Nd = ndims(q);

N = 1;
for jd = 1:Nd-1
    N = N*size(q,jd);
end

Nt = size(q,Nd);


% Reshape data
Q = reshape(q,[N,Nt]);
Wsqrt = sparse(1:N,1:N,sqrt(reshape(squeeze(Weights(:,:,1)),[N,1])));
%Wsqrt = speye(N);

% POD procedure using SVD
[U,Sig,a] = svd(Wsqrt*Q,'econ');
lambda = diag(Sig).^2;
phi = Wsqrt\U;

% Truncate modes, if desired
if numout < siz(end)
    phi = phi(:,1:numout);
    lambda = lambda(1:numout);
    a = a(:,1:numout);
else
   numout = siz(end);
end
 

% Reshape to original size
phi = reshape(phi,[siz(1:Nd-1),numout]);


end