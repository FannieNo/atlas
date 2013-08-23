
! module for testing creation of a Grid
module read_joana_module
  use grid_module
  implicit none
contains
    
  ! read_joanna_gg(grid)
  ! -----------------
 
  subroutine read_grid(g)
    type(Grid), intent(inout) :: g
    integer                   :: nnode
    integer                   :: nedge
    integer                   :: nface
    integer                   :: ncoin
    integer                   :: before1
    integer                   :: before2
    integer                   :: edge_cnt
    integer                   :: idx
    real                      :: vol
    integer                   :: p1,p2

    integer :: iface
    integer :: inode

    open(5,file='meshvol.d',access='sequential',status='old')

    read(5,*) nnode, nedge, nface, ncoin, before2, before1  !nbefore1<nbefore2

    ! Specify element type
    call g%init("LagrangeP1_Triag2D")
    
    g%nb_nodes = nnode
    g%nb_elems = 0 ! This mesh only contains edges *sad*
    g%nb_faces = nedge ! -(before2-before1+1)
    
    ! Fill in nodes and cells
    allocate(g%nodes(g%nb_nodes,g%dimension))
    do inode = 1, g%nb_nodes
      read(5,*) g%nodes(inode,1), g%nodes(inode,2), vol
    enddo

    edge_cnt = 0
    allocate(g%faces(g%nb_faces,2)) ! this 2 should be nb_nodes_per_face
    do iface = 1, g%nb_faces
      read(5,*) idx, p1, p2
      if (.true.) then
      !if (iface<before1 .or. iface>before2) then
        edge_cnt = edge_cnt + 1
        g%faces(edge_cnt,1) = p1
        g%faces(edge_cnt,2) = p2
      endif
    enddo
    g%nb_faces = edge_cnt
    
    close(5)
    
  end subroutine read_grid


  subroutine read_fields(g)
    type(Grid), intent(inout) :: g
    
    class(FunctionSpace),  pointer    :: vertices
    class(FunctionSpace),  pointer    :: faces

    type(Field),           pointer    :: V
    type(Field),           pointer    :: S

    integer                   :: nnode
    integer                   :: nedge
    integer                   :: nface
    integer                   :: ncoin
    integer                   :: before1
    integer                   :: before2
    integer                   :: edge_cnt
    integer                   :: idx
    real                      :: vol
    integer                   :: p1,p2
    integer :: dummy_int
    real    :: dummy_real
    integer :: iface
    integer :: inode
    integer :: n
    real :: Sx, Sy

    open(5,file='meshvol.d',access='sequential',status='old')
    read(5,*) nnode, nedge, nface, ncoin, before2, before1  !nbefore1<nbefore2

    ! Create a scalar field for the dual volume in the vertices
    vertices => new_ContinuousFunctionSpace("vertices","LagrangeP1", g)
    V     => vertices%add_scalar_field("dual_volume")

    do inode = 1, g%nb_nodes
      read(5,*) dummy_real, dummy_real, V%array(inode,1)
    enddo

    do iface = 1, nedge
      read(5,*) dummy_int, dummy_int, dummy_int
    enddo

    ! Setup a function space in the face centres for the face_normal
    faces => new_FaceFunctionSpace("faces", "LagrangeP0", g)
    S     => faces%add_vector_field("face_normal")

    edge_cnt = 0
    do iface = 1,nedge
      read(5,*) dummy_int, Sx, Sy
      if (.true.) then
      !if (iface<before1 .or. iface>before2) then
        edge_cnt = edge_cnt + 1
        S%array(edge_cnt,1) = Sx
        S%array(edge_cnt,2) = Sy
      endif
    enddo

    close(5)

    ! Print
    ! -----
    write(0,*) V%name," (",V%size,",",V%cols,") = "
    do n=1,V%size
      !write(0,*) V%array(n,:)
    end do
    write(0,*) S%name," (",S%size,",",S%cols,") = "
    do n=1,S%size
      !write(0,*) S%array(n,:)
    end do


    ! Write Gmsh
    open(50,file='meshvol.msh',access='sequential',status='REPLACE')
    write(50,'(A)')"$MeshFormat"
    write(50,'(A)')"2.2 0 8"
    write(50,'(A)')"$EndMeshFormat"
    write(50,'(A)')"$Nodes"
    write(50,*)g%nb_nodes
    do inode=1,g%nb_nodes
      write(50,*) inode, g%nodes(inode,1), g%nodes(inode,2), 0
    enddo
    write(50,'(A)')"$EndNodes"
    write(50,'(A)')"$Elements"
    write(50,*) g%nb_faces
    do iface=1, g%nb_faces
      ! element-number  type(1=lineP1)  nb_tags(=2)  tag1(=physical-group)  tag2(=elementary-group)  [nodes]
      if (iface<before1 .or. iface>before2) then
          write(50,*)  iface, 1, 2, 1, 2, g%faces(iface,1), g%faces(iface,2)
      else
          write(50,*)  iface, 1, 2, 1, 1, g%faces(iface,1), g%faces(iface,2)
      endif
    enddo
    write(50,'(A)')"$EndElements"
    write(50,'(A)')"$NodeData"
    write(50,*) 1                     ! one string tag:
    write(50,'(A)') '"Dual Volume"'   ! the name of the view ("A scalar view")
    write(50,*) 1                     ! one real tag:
    write(50,*) 0.0                   ! the time value (0.0)
    write(50,*) 3                     ! three integer tags:
    write(50,*) 0                     ! the time step (0; time steps always start at 0)
    write(50,*) 1                     ! 1-component (scalar) field
    write(50,*) g%nb_nodes            ! number of associated nodal values
    do inode=1,g%nb_nodes
      write(50,*) inode, V%array(inode,1)
    enddo
    write(50,'(A)')"$EndNodeData"
    write(50,'(A)')"$ElementData"
    write(50,*) 1                     ! one string tag:
    write(50,'(A)') '"face_normal"'   ! the name of the view ("A scalar view")
    write(50,*) 1                     ! one real tag:
    write(50,*) 0.0                   ! the time value (0.0)
    write(50,*) 3                     ! three integer tags:
    write(50,*) 0                     ! the time step (0; time steps always start at 0)
    write(50,*) 3                     ! 3-component (scalar) field
    write(50,*) g%nb_faces            ! number of associated nodal values
    do iface=1,g%nb_faces
      write(50,*) iface, S%array(iface,1), S%array(iface,2), 0
    enddo
    write(50,'(A)')"$EndElementData"
    close(50)
  end subroutine read_fields

  subroutine write_gmsh(g)
    type(Grid), intent(inout) :: g

    end subroutine write_gmsh
  
end module read_joana_module
! =============================================================================
! =============================================================================
! =============================================================================

! Main program
program main
  use grid_module
  use read_joana_module
  implicit none
  
  ! Declarations
  ! ------------
  type(Grid)                        :: g

  ! Execution
  ! ---------
  
  ! Read grid from example_grids module
 
  !call read_quads(g)
  call read_grid(g)
  call read_fields(g)

  ! Destruction
  ! -----------
  ! Recursively deallocate the grid, functionspaces, fields, ...
  call g%destruct()

end program main