! =============================================================
! FULL 2D JACOBI — 2D Decomposition with Cartesian Topology
! =============================================================
! Complete version featuring:
!  - MPI_Cart_create (2D process grid)
!  - MPI_Dims_create (automatic decomposition)
!  - Halo exchange on all 4 sides
!  - Convergence check with MPI_Allreduce
!
! Each process manages a rectangular subdomain.
!
! Compilation:  mpif90 -O2 -Wall -o jacobi2d_f jacobi_2d_full.f90
! Execution:    mpirun -np 4 ./jacobi2d_f
! =============================================================

program jacobi_2d
    use mpi
    implicit none

    ! ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    ! Global parameters
    ! ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    integer, parameter :: N       = 128
    real(8), parameter :: EPS     = 1.0d-5
    integer, parameter :: MAX_ITER = 5000

    ! MPI variables
    integer :: rank_world, size_world, ierr
    integer :: rank, coords(2)
    integer :: dims(2), periods(2)
    integer :: comm_cart
    integer :: north, south, west, east, dummy

    ! Subdomain dimensions
    integer :: my_row, my_col
    integer :: local_rows, local_cols
    integer :: global_row_start, global_col_start
    integer :: nr, nc   ! including ghost cells

    ! Grid arrays (1D storage, row-major via manual indexing)
    real(8), allocatable :: u(:,:), u_new(:,:)

    ! Convergence
    real(8)  :: local_change, global_change
    integer  :: iter

    ! MPI datatype for column exchange
    integer :: col_type

    ! Timing
    real(8)  :: t0, t1

    ! Loop indices
    integer :: i, j, ig, jg

    ! Tags
    integer, parameter :: TAG1 = 1, TAG2 = 2, TAG3 = 3, TAG4 = 4

    ! ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    ! MPI Initialization
    ! ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    call MPI_Init(ierr)
    call MPI_Comm_rank(MPI_COMM_WORLD, rank_world, ierr)
    call MPI_Comm_size(MPI_COMM_WORLD, size_world, ierr)

    ! ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    ! 2D Cartesian topology
    ! ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    dims(1)    = 0;  dims(2)    = 0
    periods(1) = 0;  periods(2) = 0

    call MPI_Dims_create(size_world, 2, dims, ierr)
    call MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 1, comm_cart, ierr)
    call MPI_Comm_rank(comm_cart, rank, ierr)
    call MPI_Cart_coords(comm_cart, rank, 2, coords, ierr)

    my_row = coords(1)   ! row position in the process grid
    my_col = coords(2)   ! column position

    local_rows = N / dims(1)
    local_cols = N / dims(2)

    global_row_start = my_row * local_rows
    global_col_start = my_col * local_cols

    if (rank == 0) then
        write(*,'(A,I0,A,I0)') "Global grid: ", N, "x", N
        write(*,'(A,I0,A,I0,A,I0,A)') "Processes: ", dims(1), "x", dims(2), &
                                        " (", size_world, " total)"
        write(*,'(A,I0,A,I0)') "Subdomain per process: ", local_rows, "x", local_cols
        write(*,*)
    end if

    ! ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    ! Neighbors (may be MPI_PROC_NULL on boundaries)
    ! ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    call MPI_Cart_shift(comm_cart, 0, -1, dummy, north, ierr)
    call MPI_Cart_shift(comm_cart, 0, +1, dummy, south, ierr)
    call MPI_Cart_shift(comm_cart, 1, -1, dummy, west,  ierr)
    call MPI_Cart_shift(comm_cart, 1, +1, dummy, east,  ierr)

    ! ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    ! Allocation: subdomain + ghost cells
    ! Fortran arrays: u(0:nr+1, 0:nc+1) — 1-based interior [1:nr, 1:nc]
    ! We use explicit ghost rows/cols: u(0:local_rows+1, 0:local_cols+1)
    ! ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    nr = local_rows + 2
    nc = local_cols + 2

    allocate( u(0:local_rows+1, 0:local_cols+1) )
    allocate( u_new(0:local_rows+1, 0:local_cols+1) )
    u     = 0.0d0
    u_new = 0.0d0

    ! ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    ! Boundary conditions: bottom boundary = 1.0
    ! ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    if (my_row == dims(1) - 1) then
        do j = 1, local_cols
            u(local_rows, j) = 1.0d0
        end do
    end if

    ! ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    ! MPI datatype for columns (not contiguous in Fortran either
    ! with row-major intent; here columns ARE contiguous — stride 1 in dim 1)
    ! In Fortran column-major layout, a column slice u(1:local_rows, j)
    ! is contiguous, so we do NOT need a derived type for columns.
    ! We DO need one for rows (u(i, 1:local_cols) is strided).
    !
    ! North/South exchange  → rows   → strided (MPI_Type_vector)
    ! East/West exchange    → columns → contiguous (plain MPI_DOUBLE)
    !
    ! NOTE: this is the opposite of C++ (row-major).
    ! ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

    ! Row type: local_cols elements, stride = local_rows+2 (leading dim)
    call MPI_Type_vector(local_cols, 1, local_rows+2, MPI_DOUBLE_PRECISION, &
                         col_type, ierr)
    call MPI_Type_commit(col_type, ierr)

    ! ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    ! Jacobi loop
    ! ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    t0 = MPI_Wtime()
    iter = 0
    global_change = 1.0d0

    do while (global_change > EPS .and. iter < MAX_ITER)

        ! --- North-South Halo Exchange (rows) ---
        ! In Fortran layout, u(i, 1:local_cols) is a row — strided in memory.
        ! We pass u(1,1) / u(local_rows,1) and use col_type to pick local_cols
        ! elements with stride (local_rows+2).

        ! Send row 1 north, receive into ghost row 0
        call MPI_Sendrecv( u(1, 1),          1, col_type, north, TAG1, &
                           u(0, 1),          1, col_type, north, TAG2, &
                           comm_cart, MPI_STATUS_IGNORE, ierr )

        ! Send row local_rows south, receive into ghost row local_rows+1
        call MPI_Sendrecv( u(local_rows,   1), 1, col_type, south, TAG2, &
                           u(local_rows+1, 1), 1, col_type, south, TAG1, &
                           comm_cart, MPI_STATUS_IGNORE, ierr )

        ! --- East-West Halo Exchange (columns, contiguous in Fortran) ---
        ! Send column 1 west, receive into ghost column 0
        call MPI_Sendrecv( u(1, 1),           local_rows, MPI_DOUBLE_PRECISION, west, TAG3, &
                           u(1, local_cols+1), local_rows, MPI_DOUBLE_PRECISION, east, TAG3, &
                           comm_cart, MPI_STATUS_IGNORE, ierr )

        ! Send column local_cols east, receive into ghost column local_cols+1
        call MPI_Sendrecv( u(1, local_cols),  local_rows, MPI_DOUBLE_PRECISION, east, TAG4, &
                           u(1, 0),           local_rows, MPI_DOUBLE_PRECISION, west, TAG4, &
                           comm_cart, MPI_STATUS_IGNORE, ierr )

        ! --- Jacobi Update ---
        local_change = 0.0d0

        do j = 1, local_cols
            jg = global_col_start + (j - 1)
            do i = 1, local_rows
                ig = global_row_start + (i - 1)

                ! Preserve boundary conditions
                if (ig == 0 .or. ig == N-1 .or. jg == 0 .or. jg == N-1) then
                    u_new(i, j) = u(i, j)
                    cycle
                end if

                u_new(i, j) = 0.25d0 * ( &
                    u(i-1, j) + u(i+1, j) + &
                    u(i, j-1) + u(i, j+1) )

                local_change = max(local_change, abs(u_new(i,j) - u(i,j)))
            end do
        end do

        ! --- Global Convergence Check ---
        call MPI_Allreduce(local_change, global_change, 1, &
                           MPI_DOUBLE_PRECISION, MPI_MAX, comm_cart, ierr)

        ! Swap arrays (pointer swap via intrinsic trick or explicit copy)
        u = u_new
        iter = iter + 1

        if (rank == 0 .and. mod(iter, 500) == 0) then
            write(*,'(A,I5,A,ES12.5)') "[Iter ", iter, "] max_diff = ", global_change
        end if

    end do

    t1 = MPI_Wtime()

    if (rank == 0) then
        write(*,*)
        if (global_change <= EPS) then
            write(*,'(A,I0,A)') "✓ CONVERGENCE after ", iter, " iterations"
        else
            write(*,'(A,I0,A)') "⚠ MAX_ITER reached after ", iter, " iterations"
        end if
        write(*,'(A,ES12.5)') "  Final change: ", global_change
        write(*,'(A,F8.3,A)')  "  Total time:   ", (t1 - t0), " s"
    end if

    ! ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    ! Cleanup
    ! ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    deallocate(u, u_new)
    call MPI_Type_free(col_type, ierr)
    call MPI_Comm_free(comm_cart, ierr)
    call MPI_Finalize(ierr)

end program jacobi_2d
