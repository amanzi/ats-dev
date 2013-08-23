/*
  License:
  Authors: Ethan Coon (ecoon@lanl.gov) (ATS version)

  Deformation optimization matrix
*/

#ifndef OPERATORS_MATRIX_VOLUMETRIC_DEFORMATION_HH_
#define OPERATORS_MATRIX_VOLUMETRIC_DEFORMATION_HH_


#include "Epetra_Map.h"
#include "Epetra_Operator.h"
#include "Epetra_Vector.h"
#include "Epetra_MultiVector.h"
#include "Epetra_SerialDenseVector.h"
#include "Epetra_CrsMatrix.h"

#include "Mesh.hh"
#include "composite_vector.hh"
#include "composite_matrix.hh"
#include "Matrix_PreconditionerDelegate.hh"

namespace Amanzi {
namespace Operators {

class MatrixVolumetricDeformation : public CompositeMatrix {

 public:

  MatrixVolumetricDeformation(Teuchos::ParameterList& plist,
          const Teuchos::RCP<const AmanziMesh::Mesh>& mesh);

  MatrixVolumetricDeformation(const MatrixVolumetricDeformation& other);


  // Vector space of the Matrix's domain.
  virtual Teuchos::RCP<const CompositeVectorFactory> domain() const {
    return domain_; }

  // Vector space of the Matrix's range.
  virtual Teuchos::RCP<const CompositeVectorFactory> range() const {
    return range_; }

  // Virtual copy constructor.
  virtual Teuchos::RCP<CompositeMatrix> Clone() const {
    return Teuchos::rcp(new MatrixVolumetricDeformation(*this)); }

  // Apply matrix, b <-- Ax
  virtual void Apply(const CompositeVector& x,
                     const Teuchos::Ptr<CompositeVector>& b) const;

  // Apply the inverse, x <-- A^-1 b
  virtual void ApplyInverse(const CompositeVector& b,
                            const Teuchos::Ptr<CompositeVector>& x) const;

  // This is a Normal equation, so we need to apply N^T to the rhs
  void ApplyRHS(const CompositeVector& x_cell,
                const Teuchos::Ptr<CompositeVector>& x_node,
                const Teuchos::Ptr<const AmanziMesh::Entity_ID_List>& fixed_nodes) const;

  // Sets up the solver.
  void InitializeInverse();
  void Assemble(const Teuchos::Ptr<const AmanziMesh::Entity_ID_List>& fixed_nodes);

 protected:
  void InitializeFromOptions_();
  void PreAssemble_();

 protected:
  // solver methods
  Teuchos::RCP<Matrix_PreconditionerDelegate> prec_;

  // local data
  Teuchos::RCP<CompositeVectorFactory> range_;
  Teuchos::RCP<CompositeVectorFactory> domain_;
  Teuchos::RCP<const AmanziMesh::Mesh> mesh_;
  Teuchos::ParameterList plist_;
  Teuchos::RCP<Epetra_CrsMatrix> operator_;
  Teuchos::RCP<Epetra_CrsMatrix> operatorPre_;
  Teuchos::RCP<Epetra_CrsMatrix> dVdz_;
  int max_nnode_neighbors_;

  // parameters to control optimization
  double diagonal_shift_;
  double smoothing_;
};


}
}



#endif